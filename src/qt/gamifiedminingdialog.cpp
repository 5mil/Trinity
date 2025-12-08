// Copyright (c) 2014 The Trinity developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gamifiedminingdialog.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "bitcoinunits.h"
#include "guiutil.h"
#include "guiconstants.h"

#include "main.h"
#include "miner.h"
#include "util.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QPainter>
#include <QLinearGradient>
#include <QMessageBox>

GamifiedMiningDialog::GamifiedMiningDialog(QWidget *parent) :
    QDialog(parent),
    clientModel(0),
    walletModel(0),
    isMining(false),
    miningStartTime(0),
    totalHashes(0),
    blocksFound(0)
{
    setWindowTitle(tr("Solo Mining Adventure"));
    setMinimumSize(700, 600);
    
    loadHardwareProfiles();
    setupUI();
    
    // Update timer - refresh every 2 seconds
    updateTimer = new QTimer(this);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateDisplay()));
    updateTimer->start(2000);
}

GamifiedMiningDialog::~GamifiedMiningDialog()
{
    if (isMining) {
        stopMining();
    }
}

void GamifiedMiningDialog::loadHardwareProfiles()
{
    // Predefined hardware profiles with estimated hashrates for SHA256D
    hardwareProfiles.clear();
    
    // CPU Mining
    hardwareProfiles.append({tr("Single Core CPU"), 100000, 1});           // ~100 KH/s
    hardwareProfiles.append({tr("Dual Core CPU"), 200000, 2});             // ~200 KH/s
    hardwareProfiles.append({tr("Quad Core CPU"), 400000, 4});             // ~400 KH/s
    hardwareProfiles.append({tr("8-Core CPU"), 800000, 8});                // ~800 KH/s
    
    // GPU Mining (estimated for various GPUs)
    hardwareProfiles.append({tr("Low-end GPU"), 5000000, 1});              // ~5 MH/s
    hardwareProfiles.append({tr("Mid-range GPU"), 50000000, 1});           // ~50 MH/s
    hardwareProfiles.append({tr("High-end GPU"), 200000000, 1});           // ~200 MH/s
    hardwareProfiles.append({tr("Multiple GPUs"), 500000000, 1});          // ~500 MH/s
    
    // ASIC Mining
    hardwareProfiles.append({tr("Entry ASIC"), 1000000000, 1});            // ~1 GH/s
    hardwareProfiles.append({tr("Standard ASIC"), 10000000000, 1});        // ~10 GH/s
    hardwareProfiles.append({tr("Advanced ASIC"), 100000000000, 1});       // ~100 GH/s
    
    // Custom
    hardwareProfiles.append({tr("Custom"), 0, 1});
}

void GamifiedMiningDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // ===== DIFFICULTY VISUALIZATION =====
    QGroupBox *difficultyGroup = new QGroupBox(tr("Network Difficulty Visualization"));
    QVBoxLayout *difficultyLayout = new QVBoxLayout(difficultyGroup);
    
    difficultyLabel = new QLabel(tr("Current Difficulty:"));
    difficultyLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    difficultyValueLabel = new QLabel(tr("Loading..."));
    difficultyValueLabel->setStyleSheet("font-size: 20pt; font-weight: bold; color: #0066cc;");
    
    difficultyBar = new QProgressBar();
    difficultyBar->setMinimum(0);
    difficultyBar->setMaximum(100);
    difficultyBar->setTextVisible(false);
    difficultyBar->setStyleSheet(
        "QProgressBar {"
        "   border: 2px solid grey;"
        "   border-radius: 5px;"
        "   text-align: center;"
        "   height: 30px;"
        "}"
        "QProgressBar::chunk {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00ff00, stop:0.5 #ffff00, stop:1 #ff0000);"
        "   width: 1px;"
        "}"
    );
    
    networkHashrateLabel = new QLabel(tr("Network Hashrate: Calculating..."));
    
    difficultyLayout->addWidget(difficultyLabel);
    difficultyLayout->addWidget(difficultyValueLabel);
    difficultyLayout->addWidget(difficultyBar);
    difficultyLayout->addWidget(networkHashrateLabel);
    
    mainLayout->addWidget(difficultyGroup);
    
    // ===== HARDWARE CONFIGURATION =====
    QGroupBox *hardwareGroup = new QGroupBox(tr("⚙️ Mining Hardware Configuration"));
    QGridLayout *hardwareLayout = new QGridLayout(hardwareGroup);
    
    hardwareLayout->addWidget(new QLabel(tr("Hardware Type:")), 0, 0);
    hardwareTypeCombo = new QComboBox();
    for (int i = 0; i < hardwareProfiles.size(); i++) {
        hardwareTypeCombo->addItem(hardwareProfiles[i].name);
    }
    connect(hardwareTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onHardwareChanged(int)));
    hardwareLayout->addWidget(hardwareTypeCombo, 0, 1);
    
    hardwareLayout->addWidget(new QLabel(tr("Threads:")), 1, 0);
    threadCountSpin = new QSpinBox();
    threadCountSpin->setMinimum(1);
    threadCountSpin->setMaximum(32);
    threadCountSpin->setValue(4);
    connect(threadCountSpin, SIGNAL(valueChanged(int)), this, SLOT(onThreadsChanged(int)));
    hardwareLayout->addWidget(threadCountSpin, 1, 1);
    
    hardwareLayout->addWidget(new QLabel(tr("Mining Algorithm:")), 2, 0);
    algoCombo = new QComboBox();
    algoCombo->addItem(tr("SHA256D (Bitcoin-compatible)"));
    algoCombo->addItem(tr("Scrypt"));
    algoCombo->addItem(tr("Groestl"));
    connect(algoCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onAlgoChanged(int)));
    hardwareLayout->addWidget(algoCombo, 2, 1);
    
    estimatedHashrateLabel = new QLabel(tr("Estimated Hashrate: 0 H/s"));
    estimatedHashrateLabel->setStyleSheet("font-weight: bold; color: #0066cc;");
    hardwareLayout->addWidget(estimatedHashrateLabel, 3, 0, 1, 2);
    
    mainLayout->addWidget(hardwareGroup);
    
    // ===== SOLO MINING PROBABILITY =====
    QGroupBox *probabilityGroup = new QGroupBox(tr("🎯 Solo Mining Statistics"));
    QVBoxLayout *probabilityLayout = new QVBoxLayout(probabilityGroup);
    
    yourHashrateLabel = new QLabel(tr("Your Hashrate: 0 H/s"));
    yourHashrateLabel->setStyleSheet("font-size: 12pt;");
    
    blockChanceLabel = new QLabel(tr("Chance of Finding Block: 0%"));
    blockChanceLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #ff6600;");
    
    chanceBar = new QProgressBar();
    chanceBar->setMinimum(0);
    chanceBar->setMaximum(10000); // 0.01% precision
    chanceBar->setValue(0);
    chanceBar->setFormat("%p%");
    chanceBar->setStyleSheet(
        "QProgressBar {"
        "   border: 2px solid grey;"
        "   border-radius: 5px;"
        "   text-align: center;"
        "   height: 25px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #00cc66;"
        "}"
    );
    
    expectedTimeLabel = new QLabel(tr("Expected Time to Block: ∞"));
    expectedTimeLabel->setStyleSheet("font-size: 12pt; color: #666666;");
    
    probabilityLayout->addWidget(yourHashrateLabel);
    probabilityLayout->addWidget(blockChanceLabel);
    probabilityLayout->addWidget(chanceBar);
    probabilityLayout->addWidget(expectedTimeLabel);
    
    mainLayout->addWidget(probabilityGroup);
    
    // ===== MINING STATUS =====
    QGroupBox *statusGroup = new QGroupBox(tr("📊 Mining Status"));
    QGridLayout *statusLayout = new QGridLayout(statusGroup);
    
    miningStatusLabel = new QLabel(tr("Status: Not Mining"));
    miningStatusLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    statusLayout->addWidget(miningStatusLabel, 0, 0, 1, 2);
    
    uptimeLabel = new QLabel(tr("Uptime: 0s"));
    statusLayout->addWidget(uptimeLabel, 1, 0);
    
    sharesLabel = new QLabel(tr("Hashes: 0"));
    statusLayout->addWidget(sharesLabel, 1, 1);
    
    blocksFoundLabel = new QLabel(tr("Blocks Found: 0 🏆"));
    blocksFoundLabel->setStyleSheet("font-size: 12pt; color: #00cc00;");
    statusLayout->addWidget(blocksFoundLabel, 2, 0);
    
    lastBlockLabel = new QLabel(tr("Last Block: Never"));
    statusLayout->addWidget(lastBlockLabel, 2, 1);
    
    mainLayout->addWidget(statusGroup);
    
    // ===== ACTION BUTTONS =====
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    startMiningButton = new QPushButton(tr("🚀 START SOLO MINING!"));
    startMiningButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #00cc66;"
        "   color: white;"
        "   font-size: 16pt;"
        "   font-weight: bold;"
        "   padding: 15px;"
        "   border-radius: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #00ff88;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #009944;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #666666;"
        "}"
    );
    connect(startMiningButton, SIGNAL(clicked()), this, SLOT(startMining()));
    
    stopMiningButton = new QPushButton(tr("⏹ STOP MINING"));
    stopMiningButton->setEnabled(false);
    stopMiningButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #ff6666;"
        "   color: white;"
        "   font-size: 16pt;"
        "   font-weight: bold;"
        "   padding: 15px;"
        "   border-radius: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #ff8888;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #cc4444;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #666666;"
        "}"
    );
    connect(stopMiningButton, SIGNAL(clicked()), this, SLOT(stopMining()));
    
    buttonLayout->addWidget(startMiningButton);
    buttonLayout->addWidget(stopMiningButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Initial update
    onHardwareChanged(0);
}

void GamifiedMiningDialog::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if (model) {
        updateDisplay();
    }
}

void GamifiedMiningDialog::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

double GamifiedMiningDialog::getCurrentDifficulty()
{
    if (!clientModel)
        return 1.0;
    
    // Get difficulty for current algorithm
    int algo = algoCombo->currentIndex();
    return GetDifficulty(GetLastBlockIndexForAlgo(pindexBest, algo), algo);
}

double GamifiedMiningDialog::getNetworkHashrate()
{
    // Estimate network hashrate from difficulty
    // Network hashrate ≈ difficulty * 2^32 / block_time
    double difficulty = getCurrentDifficulty();
    double blockTime = 150.0; // 2.5 minutes in seconds
    return (difficulty * 4294967296.0) / blockTime;
}

double GamifiedMiningDialog::getYourHashrate()
{
    if (isMining) {
        // Get actual hashrate from mining
        return dHashesPerSec;
    } else {
        // Get estimated hashrate from hardware profile
        int index = hardwareTypeCombo->currentIndex();
        if (index >= 0 && index < hardwareProfiles.size()) {
            return hardwareProfiles[index].hashrate * (threadCountSpin->value() / (double)hardwareProfiles[index].threads);
        }
    }
    return 0;
}

double GamifiedMiningDialog::calculateBlockChance()
{
    double yourHashrate = getYourHashrate();
    double networkHashrate = getNetworkHashrate();
    
    if (networkHashrate <= 0 || yourHashrate <= 0)
        return 0.0;
    
    // Chance per block = your_hashrate / network_hashrate
    return (yourHashrate / networkHashrate) * 100.0;
}

QString GamifiedMiningDialog::calculateExpectedTime()
{
    double chance = calculateBlockChance();
    
    if (chance <= 0)
        return tr("∞ (Never)");
    
    // Expected blocks to find one = 1 / probability
    // Time = expected_blocks * block_time
    double blockTime = 150.0; // 2.5 minutes
    double expectedBlocks = 100.0 / chance;
    int64_t seconds = (int64_t)(expectedBlocks * blockTime);
    
    return formatTime(seconds);
}

QString GamifiedMiningDialog::formatHashrate(double hashrate)
{
    if (hashrate >= 1000000000000.0) // TH/s
        return QString::number(hashrate / 1000000000000.0, 'f', 2) + " TH/s";
    else if (hashrate >= 1000000000.0) // GH/s
        return QString::number(hashrate / 1000000000.0, 'f', 2) + " GH/s";
    else if (hashrate >= 1000000.0) // MH/s
        return QString::number(hashrate / 1000000.0, 'f', 2) + " MH/s";
    else if (hashrate >= 1000.0) // KH/s
        return QString::number(hashrate / 1000.0, 'f', 2) + " KH/s";
    else
        return QString::number(hashrate, 'f', 2) + " H/s";
}

QString GamifiedMiningDialog::formatTime(int64_t seconds)
{
    if (seconds < 60)
        return QString::number(seconds) + tr(" seconds");
    else if (seconds < 3600)
        return QString::number(seconds / 60) + tr(" minutes");
    else if (seconds < 86400)
        return QString::number(seconds / 3600, 'f', 1) + tr(" hours");
    else if (seconds < 31536000)
        return QString::number(seconds / 86400, 'f', 1) + tr(" days");
    else
        return QString::number(seconds / 31536000, 'f', 1) + tr(" years");
}

void GamifiedMiningDialog::updateDifficultyVisualization()
{
    double difficulty = getCurrentDifficulty();
    
    // Update difficulty display
    difficultyValueLabel->setText(QString::number(difficulty, 'f', 2));
    
    // Scale difficulty to 0-100 for progress bar (logarithmic scale)
    // Assuming difficulty ranges from 1 to 1,000,000,000
    double logDiff = log10(difficulty);
    double logMax = 9.0; // log10(1 billion)
    int barValue = (int)((logDiff / logMax) * 100.0);
    if (barValue > 100) barValue = 100;
    if (barValue < 0) barValue = 0;
    
    difficultyBar->setValue(barValue);
    
    // Update network hashrate
    double networkHashrate = getNetworkHashrate();
    networkHashrateLabel->setText(tr("Network Hashrate: ") + formatHashrate(networkHashrate));
}

void GamifiedMiningDialog::updateStatistics()
{
    // Update your hashrate
    double yourHashrate = getYourHashrate();
    yourHashrateLabel->setText(tr("Your Hashrate: ") + formatHashrate(yourHashrate));
    
    // Update block chance
    double chance = calculateBlockChance();
    blockChanceLabel->setText(tr("Chance of Finding Block: ") + QString::number(chance, 'f', 6) + "%");
    chanceBar->setValue((int)(chance * 100.0)); // Convert to 0.01% units
    
    // Update expected time
    QString expectedTime = calculateExpectedTime();
    expectedTimeLabel->setText(tr("Expected Time to Block: ") + expectedTime);
    
    // Update mining status
    if (isMining) {
        miningStatusLabel->setText(tr("Status: ⛏️ Mining..."));
        miningStatusLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #00cc00;");
        
        int64_t uptime = GetTime() - miningStartTime;
        uptimeLabel->setText(tr("Uptime: ") + formatTime(uptime));
        
        sharesLabel->setText(tr("Hashes: ") + QString::number(totalHashes));
    } else {
        miningStatusLabel->setText(tr("Status: 💤 Not Mining"));
        miningStatusLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #cc6600;");
    }
    
    // Update blocks found
    blocksFoundLabel->setText(tr("Blocks Found: ") + QString::number(blocksFound) + " 🏆");
}

void GamifiedMiningDialog::updateDisplay()
{
    updateDifficultyVisualization();
    updateStatistics();
    
    // Update total hashes if mining
    if (isMining) {
        int64_t uptime = GetTime() - miningStartTime;
        totalHashes = (int64_t)(getYourHashrate() * uptime);
    }
}

void GamifiedMiningDialog::startMining()
{
    if (!walletModel || !clientModel) {
        QMessageBox::warning(this, tr("Cannot Start Mining"),
            tr("Wallet not loaded. Please wait for wallet to sync."));
        return;
    }
    
    // Show warning about solo mining
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        tr("Start Solo Mining?"),
        tr("Solo mining is extremely difficult and you may never find a block!\n\n"
           "Your estimated chance: %1%\n"
           "Expected time: %2\n\n"
           "Do you want to continue?")
           .arg(QString::number(calculateBlockChance(), 'f', 6))
           .arg(calculateExpectedTime()),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes)
        return;
    
    // Start mining
    int threads = threadCountSpin->value();
    int algo = algoCombo->currentIndex();
    
    // Set mining algorithm
    miningAlgo = algo;
    
    // Set thread count
    mapArgs["-genproclimit"] = QString::number(threads).toStdString();
    
    // Enable mining
    mapArgs["-gen"] = "1";
    GenerateBitcoins(true, pwalletMain);
    
    isMining = true;
    miningStartTime = GetTime();
    totalHashes = 0;
    
    startMiningButton->setEnabled(false);
    stopMiningButton->setEnabled(true);
    
    updateDisplay();
    
    LogPrintf("Gamified mining started with %d threads using algorithm %d\n", threads, algo);
}

void GamifiedMiningDialog::stopMining()
{
    // Disable mining
    mapArgs["-gen"] = "0";
    GenerateBitcoins(false, pwalletMain);
    
    isMining = false;
    
    startMiningButton->setEnabled(true);
    stopMiningButton->setEnabled(false);
    
    updateDisplay();
    
    LogPrintf("Gamified mining stopped\n");
}

void GamifiedMiningDialog::onHardwareChanged(int index)
{
    if (index < 0 || index >= hardwareProfiles.size())
        return;
    
    HardwareProfile profile = hardwareProfiles[index];
    threadCountSpin->setValue(profile.threads);
    
    double hashrate = profile.hashrate * (threadCountSpin->value() / (double)profile.threads);
    estimatedHashrateLabel->setText(tr("Estimated Hashrate: ") + formatHashrate(hashrate));
    
    updateDisplay();
}

void GamifiedMiningDialog::onThreadsChanged(int threads)
{
    int index = hardwareTypeCombo->currentIndex();
    if (index < 0 || index >= hardwareProfiles.size())
        return;
    
    HardwareProfile profile = hardwareProfiles[index];
    double hashrate = profile.hashrate * (threads / (double)profile.threads);
    estimatedHashrateLabel->setText(tr("Estimated Hashrate: ") + formatHashrate(hashrate));
    
    updateDisplay();
}

void GamifiedMiningDialog::onAlgoChanged(int index)
{
    // Algorithm changed, recalculate everything
    updateDisplay();
    
    LogPrintf("Mining algorithm changed to %d\n", index);
}
