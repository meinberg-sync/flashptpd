/*
 * @file btca.cpp
 * @note Copyright 2023, Meinberg Funkuhren GmbH & Co. KG, All rights reserved.
 * @author Thomas Behn <thomas.behn@meinberg.de>
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

#include <flashptp/selection/btca.h>
#include <flashptp/client/server.h>

namespace flashptp {
namespace selection {

int BTCA::compare(const FlashPTPServerStateDS &ds1, const FlashPTPServerStateDS &ds2)
{
    if (ds1.gmPriority1 != ds2.gmPriority1)
        return (int)ds1.gmPriority1 - (int)ds2.gmPriority1;

    if (ds1.gmClockClass != ds2.gmClockClass)
        return (int)ds1.gmClockClass - (int)ds2.gmClockClass;

    if (ds1.gmClockAccuracy != ds2.gmClockAccuracy)
        return (int)ds1.gmClockAccuracy - (int)ds2.gmClockAccuracy;

    if (ds1.gmClockVariance != ds2.gmClockVariance)
        return (int)ds1.gmClockVariance - (int)ds2.gmClockVariance;

    if (ds1.gmPriority2 != ds2.gmPriority2)
        return (int)ds1.gmPriority2 - (int)ds2.gmPriority2;

    int r = memcmp(&ds1.gmClockID, &ds2.gmClockID, sizeof(ds1.gmClockID));
    if (r != 0)
        return r;
    else
        return (int)ds1.stepsRemoved - (int)ds2.stepsRemoved;
}

std::vector<client::Server*> BTCA::select(const std::vector<client::Server*> servers, clockid_t clockID)
{
    std::vector<client::Server*> p, r;
    FlashPTPServerStateDS bsds, cds;
    client::Server *bs;
    unsigned i, j;

    p = preprocess(servers, clockID);
    if (p.empty())
        return p;

    while (r.size() < _pick) {
        for (i = 0, j = p.size(); i < p.size(); ++i) {
            if (!p[i]->serverStateDSValid())
                continue;

            if (std::find(r.begin(), r.end(), p[i]) != r.end())
                continue;

            if (j == p.size()) {
                bsds = p[i]->serverStateDS();
                j = i;
            }
            else {
                cds = p[i]->serverStateDS();
                if (compare(bsds, cds) > 0) {
                    bsds = cds;
                    j = i;
                }
            }
        }

        if (j != p.size())
            r.push_back(p[j]);
        else
            break;
    }

    postprocess(r, clockID);
    return r;
}

}
}
