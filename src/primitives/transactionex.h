// Copyright (c) 2018      The NPW developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_TRANSACTIONEX_H
#define BITCOIN_PRIMITIVES_TRANSACTIONEX_H

#include "uint256.h"
#include <list>

class CWallet;
class CWalletTx;

class TransactionStatusEx
{
public:
    TransactionStatusEx() : countsForBalance(false), sortKey(""),
		matures_in(0), status(Offline), depth(0), open_for(0), cur_num_blocks(-1)
    {
    }

    enum Status {
        Confirmed, /**< Have 6 or more confirmations (normal tx) or fully mature (mined tx) **/
        /// Normal (sent/received) transactions
        OpenUntilDate,  /**< Transaction not yet final, waiting for date */
        OpenUntilBlock, /**< Transaction not yet final, waiting for block */
        Offline,        /**< Not sent to any other nodes **/
        Unconfirmed,    /**< Not yet mined into a block **/
        Confirming,     /**< Confirmed, but waiting for the recommended number of confirmations **/
        Conflicted,     /**< Conflicts with other transaction or mempool **/
        /// Generated (mined) transactions
        Immature,       /**< Mined but waiting for maturity */
        MaturesWarning, /**< Transaction will likely not mature because no nodes have confirmed */
        NotAccepted     /**< Mined but not accepted */
    };

    /// Transaction counts towards available balance
    bool countsForBalance;
    /// Sorting key based on status
    std::string sortKey;

    /** @name Generated (mined) transactions
       @{*/
    int matures_in;
    /**@}*/

    /** @name Reported status
       @{*/
    Status status;
    int64_t depth;
    int64_t open_for; /**< Timestamp if status==OpenUntilDate, otherwise number
                      of additional blocks that need to be mined before
                      finalization */
    /**@}*/

    /** Current number of blocks (to know whether cached status is still valid) */
    int cur_num_blocks;

    //** Know when to update transaction for ix locks **/
    int cur_num_ix_locks;
};


class TransactionRecordEx
{
public:
    enum Type {
        Other,
        Generated,
        StakeMint,
        SendToAddress,
        SendToOther,
        RecvWithAddress,
        MNReward,
        RecvFromOther,
        SendToSelf,
        ZerocoinMint,
        ZerocoinSpend,
        RecvFromZerocoinSpend,
        ZerocoinSpend_Change_zNpw,
        ZerocoinSpend_FromMe,
        RecvWithObfuscation,
        ObfuscationDenominate,
        ObfuscationCollateralPayment,
        ObfuscationMakeCollaterals,
        ObfuscationCreateDenominations,
        Obfuscated
    };

    /** Number of confirmation recommended for accepting a transaction */
    static const int RecommendedNumConfirmations = 6;

    TransactionRecordEx() : hash(), time(0), type(Other), address(""), debit(0), credit(0), idx(0), blockidx(-1)
    {
    }

    TransactionRecordEx(uint256 hash, int64_t time) : hash(hash), time(time), type(Other), address(""), debit(0),
		credit(0), idx(0), blockidx(-1)
    {
    }

    TransactionRecordEx(uint256 hash, int64_t time, Type type, const std::string& address, const CAmount& debit, const CAmount& credit) : hash(hash), time(time), type(type), address(address), debit(debit), credit(credit),
		idx(0), blockidx(-1)
    {
    }

    /** Decompose CWallet transaction to model transaction records.
     */
    static std::list<TransactionRecordEx> decomposeTransaction(const CWallet* wallet, const CWalletTx& wtx);
	static bool showTransaction(const CWalletTx& wtx);
	std::string ToString() const;

    /** @name Immutable transaction attributes
      @{*/
    uint256 hash;
    int64_t time;
    Type type;
    std::string address;
    CAmount debit;
    CAmount credit;
    /**@}*/

    /** Subtransaction index, for sort key */
    int idx;

    /** Status: can change with block chain update */
    TransactionStatusEx status;

    /** blockidx: index of block contail this trasaction */
    int blockidx;

    /** Whether the transaction was sent/received with a watch-only address */
    bool involvesWatchAddress;

    /** Return the unique identifier for this transaction (part) */
    std::string getTxID() const;

    /** Return the output index of the subtransaction  */
    int getOutputIndex() const;

    /** Update status from core wallet tx.
     */
    void updateStatus(const CWalletTx& wtx);

    /** Return whether a status update is needed.
     */
    bool statusUpdateNeeded();
};

#endif // BITCOIN_PRIMITIVES_TRANSACTIONEX_H
