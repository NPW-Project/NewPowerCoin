// Copyright (c) 2018      The NPW developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addeditadrenalinenode.h"
#include "ui_addeditadrenalinenode.h"
#include "masternodelist.h"
#include "ui_masternodelist.h"
#include "masternodeconfig.h"

#include "walletdb.h"
#include "wallet.h"
#include "ui_interface.h"
#include "util.h"
#include "key.h"
#include "script/script.h"
#include "init.h"
#include "base58.h"
#include <boost/foreach.hpp>
#include <QMessageBox>
#include <QClipboard>

AddEditAdrenalineNode::AddEditAdrenalineNode(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddEditAdrenalineNode)
{
    ui->setupUi(this);

    //Labels
    ui->aliasLineEdit->setPlaceholderText(tr("Enter Masternode alias"));
    ui->addressLineEdit->setPlaceholderText(tr("Enter Masternode IP And port"));
    ui->privkeyLineEdit->setPlaceholderText(tr("Enter Masternode private key"));
    ui->txhashLineEdit->setPlaceholderText(tr("Enter 20000 IC TXID"));
    ui->outputindexLineEdit->setPlaceholderText(tr("Enter transaction output index"));
}

AddEditAdrenalineNode::~AddEditAdrenalineNode()
{
    delete ui;
}


void AddEditAdrenalineNode::on_okButton_clicked()
{
    if(ui->aliasLineEdit->text() == "")
    {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter an alias"), QMessageBox::Ok);
        return;
    }
    else if(ui->addressLineEdit->text() == "")
    {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter an ip address and port. (123.45.67.89:61472)"), QMessageBox::Ok);
        return;
    }
    else if(ui->privkeyLineEdit->text() == "")
    {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a masternode private key. This can be found using the \"masternode genkey\" command in the console"), QMessageBox::Ok);
        return;
    }
    else if(ui->txhashLineEdit->text() == "")
    {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter the transaction hash for the transaction that has 20000 coins"), QMessageBox::Ok);
        return;
    }
    else if(ui->outputindexLineEdit->text() == "")
    {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a transaction output index. This can be found using the \"masternode outputs\" command in the console"), QMessageBox::Ok);
        return;
    }
    else
    {
        std::string sAlias = ui->aliasLineEdit->text().toStdString();
        std::string sAddress = ui->addressLineEdit->text().toStdString();
        std::string sMasternodePrivKey = ui->privkeyLineEdit->text().toStdString();
        std::string sTxHash = ui->txhashLineEdit->text().toStdString();
        std::string sOutputIndex = ui->outputindexLineEdit->text().toStdString();

        boost::filesystem::path pathConfigFile = GetDataDir() / "masternode.conf";
        boost::filesystem::ofstream stream (pathConfigFile.string(), ios::out | ios::app);
        if (stream.is_open())
        {
            stream << sAlias << " " << sAddress << " " << sMasternodePrivKey << " " << sTxHash << " " << sOutputIndex;
	    stream << std::endl;
            stream.close();
        }
        masternodeConfig.add(sAlias, sAddress, sMasternodePrivKey, sTxHash, sOutputIndex);
        accept();
 
        QMessageBox::warning(this, tr("Warning"), tr("Finished to add the masternode, please restart the npw-qt client"), QMessageBox::Ok);
    }
}

void AddEditAdrenalineNode::on_cancelButton_clicked()
{
    reject();
}

void AddEditAdrenalineNode::on_AddEditAddressPasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->addressLineEdit->setText(QApplication::clipboard()->text());
}

void AddEditAdrenalineNode::on_AddEditPrivkeyPasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->privkeyLineEdit->setText(QApplication::clipboard()->text());
}

void AddEditAdrenalineNode::on_AddEditTxhashPasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->txhashLineEdit->setText(QApplication::clipboard()->text());
}
