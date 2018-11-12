// Copyright (c) 2018      The NPW developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"
#include "primitives/transactionex.h"
#include "obfuscation.h"
#include "swifttx.h"
#include "wallet.h"

#include <boost/foreach.hpp>

/* Return positive answer if transaction should be shown in list.
 */
bool TransactionRecordEx::showTransaction(const CWalletTx& wtx)
{
    if (wtx.IsCoinBase()) {
        // Ensures we show generated coins / mined transactions at depth 1
        if (!wtx.IsInMainChain()) {
            return false;
        }
    }
    return true;
}

/*
 * Decompose CWallet transaction to model transaction records.
 */
std::list<TransactionRecordEx> TransactionRecordEx::decomposeTransaction(const CWallet* wallet, const CWalletTx& wtx)
{
    std::list<TransactionRecordEx> parts;
    int64_t nTime = wtx.GetComputedTxTime();
    CAmount nCredit = wtx.GetCredit(ISMINE_ALL);
    CAmount nDebit = wtx.GetDebit(ISMINE_ALL);
    CAmount nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    std::map<std::string, std::string> mapValue = wtx.mapValue;

    if (wtx.IsCoinStake()) {
        TransactionRecordEx sub(hash, nTime);
        CTxDestination address;
        if (!ExtractDestination(wtx.vout[1].scriptPubKey, address))
            return parts;

        if (!IsMine(*wallet, address)) {
            //if the address is not yours then it means you have a tx sent to you in someone elses coinstake tx
            for (unsigned int i = 1; i < wtx.vout.size(); i++) {
                CTxDestination outAddress;
                if (ExtractDestination(wtx.vout[i].scriptPubKey, outAddress)) {
                    if (IsMine(*wallet, outAddress)) {
                        isminetype mine = wallet->IsMine(wtx.vout[i]);
                        sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
                        sub.type = TransactionRecordEx::MNReward;
                        sub.address = CBitcoinAddress(outAddress).ToString();
                        sub.credit = wtx.vout[i].nValue;
                    }
                }
            }
        } else {
            //stake reward
            isminetype mine = wallet->IsMine(wtx.vout[1]);
            sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
            sub.type = TransactionRecordEx::StakeMint;
            sub.address = CBitcoinAddress(address).ToString();
            sub.credit = nNet;
        }
        parts.push_back(sub);
    } else if (wtx.IsZerocoinSpend()) {
        // a zerocoin spend that was created by this wallet
        libzerocoin::CoinSpend zcspend = TxInToZerocoinSpend(wtx.vin[0]);
        bool fSpendFromMe = wallet->IsMyZerocoinSpend(zcspend.getCoinSerialNumber());

        //zerocoin spend outputs
        bool fFeeAssigned = false;
        for (const CTxOut txout : wtx.vout) {
            // change that was reminted as zerocoins
            if (txout.IsZerocoinMint()) {
                // do not display record if this isn't from our wallet
                if (!fSpendFromMe)
                    continue;

                TransactionRecordEx sub(hash, nTime);
                sub.type = TransactionRecordEx::ZerocoinSpend_Change_zNpw;
                sub.address = mapValue["zerocoinmint"];
                sub.debit = -txout.nValue;
                if (!fFeeAssigned) {
                    sub.debit -= (wtx.GetZerocoinSpent() - wtx.GetValueOut());
                    fFeeAssigned = true;
                }
                sub.idx = parts.size();
                parts.push_back(sub);
                continue;
            }

            string strAddress = "";
            CTxDestination address;
            if (ExtractDestination(txout.scriptPubKey, address))
                strAddress = CBitcoinAddress(address).ToString();

            // a zerocoinspend that was sent to an address held by this wallet
            isminetype mine = wallet->IsMine(txout);
            if (mine) {
                TransactionRecordEx sub(hash, nTime);
                sub.type = (fSpendFromMe ? TransactionRecordEx::ZerocoinSpend_FromMe : TransactionRecordEx::RecvFromZerocoinSpend);
                sub.debit = txout.nValue;
                sub.address = mapValue["recvzerocoinspend"];
                if (strAddress != "")
                    sub.address = strAddress;
                sub.idx = parts.size();
                parts.push_back(sub);
                continue;
            }

            // spend is not from us, so do not display the spend side of the record
            if (!fSpendFromMe)
                continue;

            // zerocoin spend that was sent to someone else
            TransactionRecordEx sub(hash, nTime);
            sub.debit = -txout.nValue;
            sub.type = TransactionRecordEx::ZerocoinSpend;
            sub.address = mapValue["zerocoinspend"];
            if (strAddress != "")
                sub.address = strAddress;
            sub.idx = parts.size();
            parts.push_back(sub);
        }
    } else if (nNet > 0 || wtx.IsCoinBase()) {
        //
        // Credit
        //
        BOOST_FOREACH (const CTxOut& txout, wtx.vout) {
            isminetype mine = wallet->IsMine(txout);
            if (mine) {
                TransactionRecordEx sub(hash, nTime);
                CTxDestination address;
                sub.idx = parts.size(); // sequence number
                sub.credit = txout.nValue;
                sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
                if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*wallet, address)) {
                    // Received by NPW Address
                    sub.type = TransactionRecordEx::RecvWithAddress;
                    sub.address = CBitcoinAddress(address).ToString();
                } else {
                    // Received by IP connection (deprecated features), or a multisignature or other non-simple transaction
                    sub.type = TransactionRecordEx::RecvFromOther;
                    sub.address = mapValue["from"];
                }
                if (wtx.IsCoinBase()) {
                    // Generated
                    sub.type = TransactionRecordEx::Generated;
                }

                parts.push_back(sub);
            }
        }
    } else {
        bool fAllFromMeDenom = true;
        int nFromMe = 0;
        bool involvesWatchAddress = false;
        isminetype fAllFromMe = ISMINE_SPENDABLE;
        BOOST_FOREACH (const CTxIn& txin, wtx.vin) {
            if (wallet->IsMine(txin)) {
                fAllFromMeDenom = fAllFromMeDenom && wallet->IsDenominated(txin);
                nFromMe++;
            }
            isminetype mine = wallet->IsMine(txin);
            if (mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if (fAllFromMe > mine) fAllFromMe = mine;
        }

        isminetype fAllToMe = ISMINE_SPENDABLE;
        bool fAllToMeDenom = true;
        int nToMe = 0;
        BOOST_FOREACH (const CTxOut& txout, wtx.vout) {
            if (wallet->IsMine(txout)) {
                fAllToMeDenom = fAllToMeDenom && wallet->IsDenominatedAmount(txout.nValue);
                nToMe++;
            }
            isminetype mine = wallet->IsMine(txout);
            if (mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if (fAllToMe > mine) fAllToMe = mine;
        }

        if (fAllFromMeDenom && fAllToMeDenom && nFromMe * nToMe) {
            parts.push_back(TransactionRecordEx(hash, nTime, TransactionRecordEx::ObfuscationDenominate, "", -nDebit, nCredit));
            parts.back().involvesWatchAddress = false; // maybe pass to TransactionRecordEx as constructor argument
        } else if (fAllFromMe && fAllToMe) {
            // Payment to self
            // TODO: this section still not accurate but covers most cases,
            // might need some additional work however

            TransactionRecordEx sub(hash, nTime);
            // Payment to self by default
            sub.type = TransactionRecordEx::SendToSelf;
            sub.address = "";

            if (mapValue["DS"] == "1") {
                sub.type = TransactionRecordEx::Obfuscated;
                CTxDestination address;
                if (ExtractDestination(wtx.vout[0].scriptPubKey, address)) {
                    // Sent to NPW Address
                    sub.address = CBitcoinAddress(address).ToString();
                } else {
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    sub.address = mapValue["to"];
                }
            } else {
                for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++) {
                    const CTxOut& txout = wtx.vout[nOut];
                    sub.idx = parts.size();

                    if (wallet->IsCollateralAmount(txout.nValue)) sub.type = TransactionRecordEx::ObfuscationMakeCollaterals;
                    if (wallet->IsDenominatedAmount(txout.nValue)) sub.type = TransactionRecordEx::ObfuscationCreateDenominations;
                    if (nDebit - wtx.GetValueOut() == OBFUSCATION_COLLATERAL) sub.type = TransactionRecordEx::ObfuscationCollateralPayment;
                }
            }

            CAmount nChange = wtx.GetChange();

            sub.debit = -(nDebit - nChange);
            sub.credit = nCredit - nChange;
            parts.push_back(sub);
            parts.back().involvesWatchAddress = involvesWatchAddress; // maybe pass to TransactionRecordEx as constructor argument
        } else if (fAllFromMe) {
            //
            // Debit
            //
            CAmount nTxFee = nDebit - wtx.GetValueOut();

            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++) {
                const CTxOut& txout = wtx.vout[nOut];
                TransactionRecordEx sub(hash, nTime);
                sub.idx = parts.size();
                sub.involvesWatchAddress = involvesWatchAddress;

                if (wallet->IsMine(txout)) {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    continue;
                }

                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address)) {
                    // Sent to NPW Address
                    sub.type = TransactionRecordEx::SendToAddress;
                    sub.address = CBitcoinAddress(address).ToString();
                } else if (txout.IsZerocoinMint()){
                    sub.type = TransactionRecordEx::ZerocoinMint;
                    sub.address = mapValue["zerocoinmint"];
                } else {
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    sub.type = TransactionRecordEx::SendToOther;
                    sub.address = mapValue["to"];
                }

                if (mapValue["DS"] == "1") {
                    sub.type = TransactionRecordEx::Obfuscated;
                }

                CAmount nValue = txout.nValue;
                /* Add fee to first output */
                if (nTxFee > 0) {
                    nValue += nTxFee;
                    nTxFee = 0;
                }
                sub.debit = -nValue;

                parts.push_back(sub);
            }
        } else {
            //
            // Mixed debit transaction, can't break down payees
            //
            parts.push_back(TransactionRecordEx(hash, nTime, TransactionRecordEx::Other, "", nNet, 0));
            parts.back().involvesWatchAddress = involvesWatchAddress;
        }
    }

    for(std::list<TransactionRecordEx>::iterator it = parts.begin(); it != parts.end(); ++it)
        it->updateStatus(wtx);

    return parts;
}

void TransactionRecordEx::updateStatus(const CWalletTx& wtx)
{
    AssertLockHeld(cs_main);
    // Determine transaction status

    // Find the block the tx is in
    CBlockIndex* pindex = NULL;
    BlockMap::iterator mi = mapBlockIndex.find(wtx.hashBlock);
    if (mi != mapBlockIndex.end())
        pindex = (*mi).second;

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
        (pindex ? pindex->nHeight : std::numeric_limits<int>::max()),
        (wtx.IsCoinBase() ? 1 : 0),
        wtx.nTimeReceived,
        idx);
    status.countsForBalance = wtx.IsTrusted() && !(wtx.GetBlocksToMaturity() > 0);
    status.depth = wtx.GetDepthInMainChain();
    status.cur_num_blocks = chainActive.Height();
    status.cur_num_ix_locks = nCompleteTXLocks;

    if (!IsFinalTx(wtx, chainActive.Height() + 1)) {
        if (wtx.nLockTime < LOCKTIME_THRESHOLD) {
            status.status = TransactionStatusEx::OpenUntilBlock;
            status.open_for = wtx.nLockTime - chainActive.Height();
        } else {
            status.status = TransactionStatusEx::OpenUntilDate;
            status.open_for = wtx.nLockTime;
        }
    }
    // For generated transactions, determine maturity
    else if (type == TransactionRecordEx::Generated || type == TransactionRecordEx::StakeMint || type == TransactionRecordEx::MNReward) {
        if (wtx.GetBlocksToMaturity() > 0) {
            status.status = TransactionStatusEx::Immature;

            if (wtx.IsInMainChain()) {
                status.matures_in = wtx.GetBlocksToMaturity();

                // Check if the block was requested by anyone
                if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                    status.status = TransactionStatusEx::MaturesWarning;
            } else {
                status.status = TransactionStatusEx::NotAccepted;
            }
        } else {
            status.status = TransactionStatusEx::Confirmed;
        }
    } else {
        if (status.depth < 0) {
            status.status = TransactionStatusEx::Conflicted;
        } else if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0) {
            status.status = TransactionStatusEx::Offline;
        } else if (status.depth == 0) {
            status.status = TransactionStatusEx::Unconfirmed;
        } else if (status.depth < RecommendedNumConfirmations) {
            status.status = TransactionStatusEx::Confirming;
        } else {
            status.status = TransactionStatusEx::Confirmed;
        }
    }
}

bool TransactionRecordEx::statusUpdateNeeded()
{
    AssertLockHeld(cs_main);
    return status.cur_num_blocks != chainActive.Height() || status.cur_num_ix_locks != nCompleteTXLocks;
}

std::string TransactionRecordEx::getTxID() const
{
    return hash.ToString();
}

int TransactionRecordEx::getOutputIndex() const
{
    return idx;
}

std::string formatTxType(const TransactionRecordEx::Type type)
{
    switch (type) {
		case TransactionRecordEx::RecvWithAddress:
			return std::string("Received with");
		case TransactionRecordEx::MNReward:
			return std::string("Masternode Reward");
		case TransactionRecordEx::RecvFromOther:
			return std::string("Received from");
		case TransactionRecordEx::RecvWithObfuscation:
			return std::string("Received via Obfuscation");
		case TransactionRecordEx::SendToAddress:
		case TransactionRecordEx::SendToOther:
			return std::string("Sent to");
		case TransactionRecordEx::SendToSelf:
			return std::string("Payment to yourself");
		case TransactionRecordEx::StakeMint:
			return std::string("Minted");
		case TransactionRecordEx::Generated:
			return std::string("Mined");
		case TransactionRecordEx::ObfuscationDenominate:
			return std::string("Obfuscation Denominate");
		case TransactionRecordEx::ObfuscationCollateralPayment:
			return std::string("Obfuscation Collateral Payment");
		case TransactionRecordEx::ObfuscationMakeCollaterals:
			return std::string("Obfuscation Make Collateral Inputs");
		case TransactionRecordEx::ObfuscationCreateDenominations:
			return std::string("Obfuscation Create Denominations");
		case TransactionRecordEx::Obfuscated:
			return std::string("Obfuscated");
		case TransactionRecordEx::ZerocoinMint:
			return std::string("Converted NPW to zNPW");
		case TransactionRecordEx::ZerocoinSpend:
			return std::string("Spent zNPW");
		case TransactionRecordEx::RecvFromZerocoinSpend:
			return std::string("Received NPW from zNPW");
		case TransactionRecordEx::ZerocoinSpend_Change_zNpw:
			return std::string("Minted Change as zNPW from zNPW Spend");
		case TransactionRecordEx::ZerocoinSpend_FromMe:
			return std::string("Converted zNPW to NPW");

		default:
			return std::string("Unknown");
    }
}

std::string TransactionRecordEx::ToString() const
{
    std::string str;

    str += strprintf("txid : %s, type : %s, type value : %d, Address : %s, status : %d, Debit : %ld, Credit : %ld, time : %ld, index : %d, confirmations : %ld", 
					hash.ToString(),
					formatTxType(type),
					type,
					address,
					status.status,
					debit,
					credit,
					time,
					idx,
					status.depth);
    return str;
}
