#include "FlycastCommandParser.hpp"
#include "hal/MapleBus/MaplePacket.hpp"

#include <stdio.h>
#include <cctype>
#include <cstring>
#include <string>
#include <cstdlib>

// TODO: this is not the best way to forward declare here
extern "C" {
    void direct_write_cdc(const char *buf, int length);
}

// Format: X[modifier-char]<cmd-data>\n
// This parser must always return a single line of data

// Simple definition of a transmitter which just echos status and received data
class FlycastEchoTransmitter : public Transmitter
{
public:
    virtual void txStarted(std::shared_ptr<const Transmission> tx) final
    {}

    virtual void txFailed(bool writeFailed,
                          bool readFailed,
                          std::shared_ptr<const Transmission> tx) final
    {
        if (writeFailed)
        {
            printf("*failed write\n");
        }
        else
        {
            printf("*failed read\n");
        }
    }

    virtual void txComplete(std::shared_ptr<const MaplePacket> packet,
                            std::shared_ptr<const Transmission> tx) final
    {
        char buf[64];
        snprintf(buf, 64,
            "%02hhX %02hhX %02hhX %02hhX",
            packet->frame.command,
            packet->frame.recipientAddr,
            packet->frame.senderAddr,
            packet->frame.length);

        direct_write_cdc(buf, strlen(buf));

        for (uint32_t p : packet->payload)
        {
            snprintf(buf, 64, " %08lX", p);
            direct_write_cdc(buf, strlen(buf));
        }

        direct_write_cdc("\n", 1);
    }
} flycastEchoTransmitter;

FlycastCommandParser::FlycastCommandParser(
    SystemIdentification& identification,
    std::shared_ptr<PrioritizedTxScheduler>* schedulers,
    const uint8_t* senderAddresses,
    uint32_t numSenders,
    const std::vector<std::shared_ptr<PlayerData>>& playerData,
    const std::vector<std::shared_ptr<DreamcastMainNode>>& nodes
) :
    mIdentification(identification),
    mSchedulers(schedulers),
    mSenderAddresses(senderAddresses),
    mNumSenders(numSenders),
    mPlayerData(playerData),
    nodes(nodes)
{}

const char* FlycastCommandParser::getCommandChars()
{
    // X is reserved for command from flycast emulator
    return "X";
}

void FlycastCommandParser::submit(const char* chars, uint32_t len)
{
    if (len == 0)
    {
        // This shouldn't happen, but handle it regardless
        return;
    }

    bool valid = false;
    const char* eol = chars + len;
    std::vector<uint32_t> words;
    const char* iter = chars + 1; // Skip past 'X' (implied)

    // left strip
    while (iter < eol && std::isspace(*iter))
    {
        ++iter;
    }
    // right strip
    while (iter < eol && std::isspace(*(eol - 1)))
    {
        --eol;
    }

    // Check for special commanding
    if (iter < eol)
    {
        switch(*iter)
        {
            // Either X- to reset all or one of {X-0, X-1, X-2, X-3} to reset a specific player
            case '-':
            {
                // Remove minus
                ++iter;
                int idx = -1;
                if (iter < eol)
                {
                    std::string number;
                    number.assign(iter, eol - iter);
                    if (0 == sscanf(iter, "%i", &idx))
                    {
                        idx = -1;
                    }
                }

                // Reset screen data
                if (idx < 0)
                {
                    // all
                    int count = 0;
                    for (std::shared_ptr<PlayerData>& playerData : mPlayerData)
                    {
                        ++count;
                        playerData->screenData.resetToDefault();
                    }
                    printf("%i\n", count);
                }
                else if (static_cast<std::size_t>(idx) < mPlayerData.size())
                {
                    mPlayerData[idx]->screenData.resetToDefault();
                    printf("1\n");
                }
                else
                {
                    printf("0\n");

                }
            }
            return;

            // XP [0-4] [0-4]
            case 'P' :
            {
                // Remove P
                ++iter;
                int idxin = -1;
                int idxout = -1;
                if (2 == sscanf(iter, "%i %i", &idxin, &idxout) &&
                    idxin >= 0 &&
                    static_cast<std::size_t>(idxin) < mPlayerData.size() &&
                    idxout >= 0 &&
                    static_cast<std::size_t>(idxout) < ScreenData::NUM_DEFAULT_SCREENS)
                {
                    mPlayerData[idxin]->screenData.setDataToADefault(idxout);
                    printf("1\n");
                }
                else
                {
                    printf("0\n");
                }
            }
            return;

            // XS to return serial
            case 'S' :
            {
                char buffer[mIdentification.getSerialSize() + 1] = {0};
                mIdentification.getSerial(buffer, sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';
                printf("%s\n", buffer);
            }
            return;

            // X?0, X?1, X?2, or X?3 will print summary for the given node index
            case '?' :
            {
                // Remove question mark
                ++iter;
                int idx = -1;
                if (iter < eol)
                {
                    std::string number;
                    number.assign(iter, eol - iter);
                    if (0 == sscanf(iter, "%i", &idx))
                    {
                        idx = -1;
                    }
                }

                if (idx >= 0 && static_cast<std::size_t>(idx) < nodes.size())
                {
                    nodes[idx]->printSummary();
                }
                else
                {
                    printf("NULL\n");
                }
            }
            return;

            // Reserved
            case ' ': // Fall through
            case '0': // Fall through
            case '1': // Fall through
            case '2': // Fall through
            case '3': // Fall through
            case '4': // Fall through
            case '5': // Fall through
            case '6': // Fall through
            case '7': // Fall through
            case '8': // Fall through
            case '9': // Fall through
            case 'a': // Fall through
            case 'b': // Fall through
            case 'c': // Fall through
            case 'd': // Fall through
            case 'e': // Fall through
            case 'f': // Fall through
            case 'A': // Fall through
            case 'B': // Fall through
            case 'C': // Fall through
            case 'D': // Fall through
            case 'E': // Fall through
            case 'F': // Fall through

            // No special case
            default: break;
        }
    }

    while (iter < eol)
    {
        uint32_t word = 0;
        uint32_t i = 0;
        while (i < 8 && iter < eol)
        {
            char v = *iter++;
            uint_fast8_t value = 0;

            if (v >= '0' && v <= '9')
            {
                value = v - '0';
            }
            else if (v >= 'a' && v <= 'f')
            {
                value = v - 'a' + 0xa;
            }
            else if (v >= 'A' && v <= 'F')
            {
                value = v - 'A' + 0xA;
            }
            else
            {
                // Ignore this character
                continue;
            }

            // Apply value into current word
            word |= (value << ((8 - i) * 4 - 4));
            ++i;
        }

        // Invalid if a partial word was given
        valid = ((i == 8) || (i == 0));

        if (i == 8)
        {
            words.push_back(word);
        }
    }

    if (valid)
    {
        MaplePacket packet(&words[0], words.size());
        if (packet.isValid())
        {
            uint8_t sender = packet.frame.senderAddr;
            int32_t idx = -1;
            const uint8_t* senderAddress = mSenderAddresses;

            if (mNumSenders == 1)
            {
                // Single player special case - always send to the one available, regardless of address
                idx = 0;
                packet.frame.senderAddr = *senderAddress;
                packet.frame.recipientAddr = (packet.frame.recipientAddr & 0x3F) | *senderAddress;
            }
            else
            {
                for (uint32_t i = 0; i < mNumSenders && idx < 0; ++i, ++senderAddress)
                {
                    if (sender == *senderAddress)
                    {
                        idx = i;
                    }
                }
            }

            if (idx >= 0)
            {
                mSchedulers[idx]->add(
                    PrioritizedTxScheduler::EXTERNAL_TRANSMISSION_PRIORITY,
                    PrioritizedTxScheduler::TX_TIME_ASAP,
                    &flycastEchoTransmitter,
                    packet,
                    true);
            }
            else
            {
                printf("*failed invalid sender\n");
            }
        }
        else
        {
            printf("*failed packet invalid\n");
        }
    }
    else
    {
        printf("*failed missing data\n");
    }
}

void FlycastCommandParser::printHelp()
{
    printf("X: commands from a flycast emulator\n");
}
