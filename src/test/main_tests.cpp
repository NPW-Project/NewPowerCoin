// Copyright (c) 2014 The Bitcoin Core developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2018      The NPW developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/transaction.h"
#include "main.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(main_tests)

void CheckBlockValue(int nHeight, CAmount nSubsidy, CAmount nExpectedValue) {
    int nTail = nHeight - nHeight / 21600 * 21600;
    if (nHeight > Params().LAST_POW_BLOCK() && nTail >= 0 && nTail < 15) {
        BOOST_CHECK(nSubsidy == 1000 * COIN);
    } else {
        BOOST_CHECK(nSubsidy == nExpectedValue);
    }
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    CAmount nSum = 0;
    for (int nHeight = 0; nHeight < 1; nHeight += 1) {
        /* premine in block 1 (700,000 NPW) */
        CAmount nSubsidy = GetBlockValue(nHeight);
        CheckBlockValue(nHeight, nSubsidy, 7000000 * COIN);
        nSum += nSubsidy;
    }

    for (int nHeight = 1; nHeight < 2000; nHeight += 1) {
        /* premine in block 1 (700,000 NPW) */
        CAmount nSubsidy = GetBlockValue(nHeight);
        CheckBlockValue(nHeight, nSubsidy, 1 * COIN);
        nSum += nSubsidy;
    }

    for (int nHeight = 2000; nHeight < 300000; nHeight += 1) {
        /* PoW Phase and POS Phase One */
        CAmount nSubsidy = GetBlockValue(nHeight);
        CheckBlockValue(nHeight, nSubsidy, 100 * COIN);
        nSum += nSubsidy;
    }

    for (int nHeight = 300000; nHeight < 1000000; nHeight += 1) {
        /* PoS Phase Two */
        CAmount nSubsidy = GetBlockValue(nHeight);
        CheckBlockValue(nHeight, nSubsidy, 50 * COIN);
        nSum += nSubsidy;
    }

    for (int nHeight = 1000000; nHeight < 2000000; nHeight += 1) {
        /* POS Phase Three */
        CAmount nSubsidy = GetBlockValue(nHeight);
        CheckBlockValue(nHeight, nSubsidy, 25 * COIN);
        nSum += nSubsidy;
    }

    for (int nHeight = 2000000; nHeight < 3000000; nHeight += 1) {
        /* POS Phase Four */
        CAmount nSubsidy = GetBlockValue(nHeight);
        CheckBlockValue(nHeight, nSubsidy, 12.5 * COIN);
        nSum += nSubsidy;
    }

    // nHeight >= 3000000, nSubsidy=6.25
    BOOST_CHECK(nSum == 11128837400000000ULL);
}

BOOST_AUTO_TEST_SUITE_END()
