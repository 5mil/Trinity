// Copyright (c) 2014 The Trinity developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GAMIFIEDMININGDIALOG_H
#define GAMIFIEDMININGDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QComboBox>
#include <QSpinBox>

class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
QT_END_NAMESPACE

/** Gamified Solo Mining Dialog */
class GamifiedMiningDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GamifiedMiningDialog(QWidget *parent = 0);
    ~GamifiedMiningDialog();

    void setClientModel(ClientModel *model);
    void setWalletModel(WalletModel *model);

private:
    ClientModel *clientModel;
    WalletModel *walletModel;
    
    // UI Elements
    QLabel *difficultyLabel;
    QLabel *difficultyValueLabel;
    QLabel *networkHashrateLabel;
    QLabel *yourHashrateLabel;
    QLabel *blockChanceLabel;
    QLabel *expectedTimeLabel;
    QLabel *blocksFoundLabel;
    QLabel *lastBlockLabel;
    
    QProgressBar *difficultyBar;
    QProgressBar *chanceBar;
    QPushButton *startMiningButton;
    QPushButton *stopMiningButton;
    
    // Hardware configuration
    QComboBox *hardwareTypeCombo;
    QSpinBox *threadCountSpin;
    QLabel *estimatedHashrateLabel;
    
    // Mining algorithm selection
    QComboBox *algoCombo;
    
    // Statistics
    QLabel *miningStatusLabel;
    QLabel *uptimeLabel;
    QLabel *sharesLabel;
    
    // Timer for updates
    QTimer *updateTimer;
    
    // Internal state
    bool isMining;
    int64_t miningStartTime;
    int64_t totalHashes;
    int blocksFound;
    
    // Hardware profiles
    struct HardwareProfile {
        QString name;
        double hashrate; // hashes per second
        int threads;
    };
    
    QList<HardwareProfile> hardwareProfiles;
    
    void setupUI();
    void loadHardwareProfiles();
    double calculateBlockChance();
    QString calculateExpectedTime();
    double getCurrentDifficulty();
    double getNetworkHashrate();
    double getYourHashrate();
    void updateStatistics();
    void updateDifficultyVisualization();
    QString formatHashrate(double hashrate);
    QString formatTime(int64_t seconds);

private slots:
    void startMining();
    void stopMining();
    void updateDisplay();
    void onHardwareChanged(int index);
    void onThreadsChanged(int threads);
    void onAlgoChanged(int index);
};

#endif // GAMIFIEDMININGDIALOG_H
