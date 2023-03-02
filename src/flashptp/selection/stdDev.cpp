/*
 * @file stdDev.cpp
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

#include <flashptp/selection/stdDev.h>
#include <flashptp/client/server.h>

namespace flashptp {
namespace selection {

std::vector<client::Server*> StdDev::select(const std::vector<client::Server*> servers, clockid_t clockID)
{
    std::vector<client::Server*> p, r;
    client::Server *bs;
    int64_t bsd, csd;
    unsigned i, j;

    p = preprocess(servers, clockID);
    while (r.size() < _pick) {
        for (i = 0, j = p.size(); i < p.size(); ++i) {
            csd = p[i]->stdDev();
            if (csd == INT64_MAX)
                continue;

            if (std::find(r.begin(), r.end(), p[i]) != r.end())
                continue;

            if (j == p.size() || csd < bsd) {
                bsd = csd;
                j = i;
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
