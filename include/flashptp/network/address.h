/*
 * @file address.h
 * @note Copyright (C) 2023 Thomas Behn <thomas.behn@meinberg.de>
 *
 * The class Address represents any MAC, IPv4 or IPv6 address within flashPTP.
 * Addresses can be converted to and from string and to and from struct sockaddr.
 *
 * =============================================================================
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the “Software”),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * =============================================================================
 *
 */

#ifndef INCLUDE_FLASHPTP_NETWORK_ADDRESS_H_
#define INCLUDE_FLASHPTP_NETWORK_ADDRESS_H_

#include <flashptp/common/defines.h>

namespace flashptp {
namespace network {

class Address
{
public:
    inline Address()
    {
        memset(&_saddr, 0, sizeof(_saddr));
        _saddr.ss_family = AF_UNSPEC;
    }

    inline Address(const struct sockaddr_storage *saddr, const struct sockaddr_storage *saddrPrefix = nullptr)
    {
        _saddr = *saddr;
        if (_saddr.ss_family != AF_PACKET && _saddr.ss_family != AF_INET && _saddr.ss_family != AF_INET6)
            _saddr.ss_family = AF_UNSPEC;
        if (saddrPrefix)
            _prefix = saddrToPrefix(saddrPrefix);
        _shortStr = Address::saddrToStr(&_saddr);
        _str = _shortStr;
        if (_prefix > 0)
            _str += "/" + std::to_string(_prefix);
    }

    inline Address(const std::string &str)
    {
        memset(&_saddr, 0, sizeof(_saddr));
        auto pos = str.find('/');
        Address::saddrFromStr(str.substr(0, pos).c_str(), &_saddr);
        if (_saddr.ss_family != AF_PACKET && _saddr.ss_family != AF_INET && _saddr.ss_family != AF_INET6)
            _saddr.ss_family = AF_UNSPEC;
        if (pos != std::string::npos && pos < (str.length() - 1)) {
            _shortStr = str.substr(0, pos);
            _prefix = std::strtol(str.substr(pos + 1).c_str(), nullptr, 10);
        }
        else
            _shortStr = str;
        _str = _shortStr;
        if (_prefix > 0)
            _str += "/" + std::to_string(_prefix);
    }

    inline static const char *familyToStr(int family)
    {
        switch (family) {
        case AF_PACKET: return "LL2 (IEEE 802.3)";
        case AF_INET: return "IPv4";
        case AF_INET6: return "IPv6";
        default: return "Unknown";
        }
    }

    inline static std::string macToStr(const struct sockaddr_ll *sll)
    {
        char buf[18];
        for (int i = 0; i < 6; ++i) {
            snprintf(&buf[i * 2 + i], sizeof(buf) - i * 2 + i, "%02x", sll->sll_addr[i]);
            buf[i * 2 + i + 2] = (i < 5) ? ':' : 0;
        }
        return std::string(buf);
    }

    inline static std::string ip4ToStr(const struct sockaddr_in *sin)
    {
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sin->sin_addr.s_addr, buf, sizeof(buf));
        return std::string(buf);
    }

    inline static std::string ip6ToStr(const struct sockaddr_in6 *sin6)
    {
        char buf[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, sin6->sin6_addr.s6_addr, buf, sizeof(buf));
        return std::string(buf);
    }

    inline static std::string saddrToStr(const struct sockaddr_storage *saddr)
    {
        switch (saddr->ss_family) {
        case AF_PACKET: return Address::macToStr((const struct sockaddr_ll*)saddr);
        case AF_INET: return Address::ip4ToStr((const struct sockaddr_in*)saddr);
        case AF_INET6: return Address::ip6ToStr((const struct sockaddr_in6*)saddr);
        default: return std::string("Unknown");
        }
    }

    inline static int saddrToPrefix(const struct sockaddr_storage *saddr)
    {
        struct sockaddr_in *sin;
        struct sockaddr_in6 *sin6;
        int prefix = 0;
        unsigned i;

        if (!saddr)
            return 0;

        switch (saddr->ss_family) {
        case AF_INET:
            sin = (struct sockaddr_in*)saddr;
            for (i = 0; i < 32; ++i) {
                if (sin->sin_addr.s_addr & (1UL << i))
                    ++prefix;
                else
                    break;
            }
            break;

        case AF_INET6:
            sin6 = (struct sockaddr_in6*)saddr;
            for (i = 0; i < 128; ++i) {
                if (sin6->sin6_addr.s6_addr[i / 8] & (1UL << (i % 8)))
                    ++prefix;
                else
                    break;
            }
            break;

        default: break;
        }

        return prefix;
    }

    inline static bool macFromStr(const char *str, struct sockaddr_ll *sll = nullptr)
    {
        int a, b, c, d, e, f;
        bool valid = false;

        if (sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x", &a, &b, &c, &d, &e, &f) == 6) {
            if (sll) {
                sll->sll_family = AF_PACKET;
                sll->sll_addr[0] = a;
                sll->sll_addr[1] = b;
                sll->sll_addr[2] = c;
                sll->sll_addr[3] = d;
                sll->sll_addr[4] = e;
                sll->sll_addr[5] = f;
            }
            valid = true;
        }
        else if (sll)
            sll->sll_family = AF_UNSPEC;

        return valid;
    }

    inline static bool ip4FromStr(const char *str, struct sockaddr_in *sin = nullptr)
    {
        bool valid = false;
        in_addr_t addr;

        if (inet_pton(AF_INET, str, &addr) == 1) {
            if (sin) {
                sin->sin_family = AF_INET;
                sin->sin_addr.s_addr = addr;
            }
            valid = true;
        }
        else if (sin)
            sin->sin_family = AF_UNSPEC;

        return valid;
    }

    inline static bool ip6FromStr(const char *str, struct sockaddr_in6 *sin6 = nullptr)
    {
        bool valid = false;
        uint8_t addr[16];

        if (inet_pton(AF_INET6, str, addr) == 1) {
            if (sin6) {
                sin6->sin6_family = AF_INET6;
                memcpy(sin6->sin6_addr.s6_addr, addr, sizeof(addr));
            }
            valid = true;
        }
        else if (sin6)
            sin6->sin6_family = AF_UNSPEC;

        return valid;
    }

    inline static bool saddrFromStr(const char *str, struct sockaddr_storage *saddr = nullptr)
    {
        if (Address::macFromStr(str, (struct sockaddr_ll*)saddr))
            return true;
        else if (Address::ip4FromStr(str, (struct sockaddr_in*)saddr))
            return true;
        else
            return Address::ip6FromStr(str, (struct sockaddr_in6*)saddr);
    }

    inline bool valid() const { return _saddr.ss_family != AF_UNSPEC; }

    inline bool equals(const struct sockaddr_storage *saddr) const
    {
        if (saddr->ss_family != _saddr.ss_family)
            return false;
        if (saddr->ss_family == AF_PACKET)
            return memcmp(((struct sockaddr_ll*)saddr)->sll_addr,
                    ((struct sockaddr_ll*)&_saddr)->sll_addr, 6) == 0;
        else if (saddr->ss_family == AF_INET)
            return ((struct sockaddr_in*)saddr)->sin_addr.s_addr ==
                    ((struct sockaddr_in*)&_saddr)->sin_addr.s_addr;
        else if (saddr->ss_family == AF_INET6)
            return memcmp(((struct sockaddr_in6*)saddr)->sin6_addr.s6_addr,
                    ((struct sockaddr_in6*)&_saddr)->sin6_addr.s6_addr, 16) == 0;
        else
            return false;
    }

    inline void saddr(struct sockaddr_storage *saddr) const { *saddr = _saddr; }
    inline const struct sockaddr_storage &saddr() const { return _saddr; }
    inline unsigned family() const { return _saddr.ss_family; }
    inline int prefix() const { return _prefix; }
    inline const std::string &shortStr() const { return _shortStr; }
    inline const std::string &str() const { return _str; }

private:
    struct sockaddr_storage _saddr;
    int _prefix{ 0 };
    std::string _shortStr;
    std::string _str;
};

}
}

#endif /* INCLUDE_FLASHPTP_NETWORK_ADDRESS_H_ */
