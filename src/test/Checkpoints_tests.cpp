// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Unit tests for block-chain checkpoints
//

#include "checkpoints.h"

#include "uint256.h"

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_AUTO_TEST_SUITE(Checkpoints_tests)

BOOST_AUTO_TEST_CASE(sanity)
{
    uint256 p0 = uint256("0x0000094d510b11b1b902e77b10f793a2b43ad81d1db9884f1ef26c773711965f");
    BOOST_CHECK(Checkpoints::CheckBlock(0, p0));


    // Wrong hashes at checkpoints should fail:
    // BOOST_CHECK(!Checkpoints::CheckBlock(259201, p623933));
    // BOOST_CHECK(!Checkpoints::CheckBlock(623933, p259201));

    // ... but any hash not at a checkpoint should succeed:
    // BOOST_CHECK(Checkpoints::CheckBlock(259201+1, p623933));
    // BOOST_CHECK(Checkpoints::CheckBlock(623933+1, p259201));

    // BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate() >= 623933);
}

BOOST_AUTO_TEST_SUITE_END()
