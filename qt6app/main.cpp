#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QScrollBar>
#include <QPalette>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QStyleFactory>
#include <QTimer>
#include <QFontDatabase>
#include <QThread>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QSpinBox>
#include <QDateTime>

class PasswordDialog : public QDialog {
public:
    PasswordDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Enter Password");
        QFormLayout *layout = new QFormLayout(this);

        passwordEdit = new QLineEdit;
        passwordEdit->setEchoMode(QLineEdit::Password);
        confirmEdit = new QLineEdit;
        confirmEdit->setEchoMode(QLineEdit::Password);

        layout->addRow("Password:", passwordEdit);
        layout->addRow("Confirm Password:", confirmEdit);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &PasswordDialog::validate);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        layout->addWidget(buttonBox);
    }

    QString password() const { return passwordEdit->text(); }

private:
    void validate() {
        if (passwordEdit->text() != confirmEdit->text()) {
            QMessageBox::warning(this, "Error", "Passwords do not match!");
            return;
        }
        if (passwordEdit->text().isEmpty()) {
            QMessageBox::warning(this, "Error", "Password cannot be empty!");
            return;
        }
        accept();
    }

    QLineEdit *passwordEdit;
    QLineEdit *confirmEdit;
};

class CommandRunner : public QObject {
    Q_OBJECT
public:
    explicit CommandRunner(QObject *parent = nullptr) : QObject(parent) {}

signals:
    void commandStarted(const QString &command);
    void commandOutput(const QString &output);
    void commandFinished(bool success);

public slots:
    void runCommand(const QString &command, const QStringList &args = QStringList(), bool asRoot = false) {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);

        QString fullCommand;
        if (asRoot) {
            fullCommand = "sudo " + command;
            if (!args.isEmpty()) {
                fullCommand += " " + args.join(" ");
            }
        } else {
            fullCommand = command;
            if (!args.isEmpty()) {
                fullCommand += " " + args.join(" ");
            }
        }

        emit commandStarted(fullCommand);

        if (asRoot) {
            QStringList fullArgs;
            fullArgs << command;
            fullArgs += args;
            
            process.start("sudo", fullArgs);
            if (!process.waitForStarted()) {
                emit commandOutput("Failed to start sudo command\n");
                emit commandFinished(false);
                return;
            }
            
            process.write((m_sudoPassword + "\n").toUtf8());
            process.closeWriteChannel();
        } else {
            process.start(command, args);
        }

        if (!process.waitForStarted()) {
            emit commandOutput("Failed to start command: " + fullCommand + "\n");
            emit commandFinished(false);
            return;
        }

        while (process.waitForReadyRead()) {
            QByteArray output = process.readAll();
            emit commandOutput(QString::fromUtf8(output));
        }

        process.waitForFinished();

        if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
            emit commandFinished(true);
        } else {
            emit commandOutput(QString("Command failed with exit code %1\n").arg(process.exitCode()));
            emit commandFinished(false);
        }
    }

    void setSudoPassword(const QString &password) {
        m_sudoPassword = password;
        m_sudoPassword.replace("'", "'\\''");
    }

private:
    QString m_sudoPassword;
};

class CachyOSInstaller : public QMainWindow {
    Q_OBJECT

public:
    CachyOSInstaller(QWidget *parent = nullptr) : QMainWindow(parent) {
        // Set dark theme
        qApp->setStyle(QStyleFactory::create("Fusion"));
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(53,53,53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(25,25,25));
        darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53,53,53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(darkPalette);

        // Create main widgets
        QWidget *centralWidget = new QWidget;
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        // ASCII art title - EXACTLY as you provided
        QLabel *titleLabel = new QLabel;
        titleLabel->setText(
            "<span style='color:#ff0000;'>░█████╗░██╗░░░░░░█████╗░██╗░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗<br>"
            "██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝<br>"
            "██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░<br>"
            "██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗<br>"
            "╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝<br>"
            "░╚════╝░╚══════╝╚═╝░░░░░░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░</span><br>"
            "<span style='color:#00ffff;'>CachyOS Btrfs Installer v1.0</span>"
        );
        titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(titleLabel);

        // Progress bar with log button
        QHBoxLayout *progressLayout = new QHBoxLayout;
        progressBar = new QProgressBar;
        progressBar->setRange(0, 100);
        progressBar->setTextVisible(true);

        // Customize progress bar with cyan gradient
        QString progressStyle =
        "QProgressBar {"
        "    border: 2px solid grey;"
        "    border-radius: 5px;"
        "    text-align: center;"
        "    background: #252525;"
        "}"
        "QProgressBar::chunk {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "        stop:0 #00ffff, stop:1 #00aaff);"
        "}";
        progressBar->setStyleSheet(progressStyle);

        QPushButton *logButton = new QPushButton("Log");
        logButton->setFixedWidth(60);
        connect(logButton, &QPushButton::clicked, this, &CachyOSInstaller::toggleLog);

        progressLayout->addWidget(progressBar, 1);
        progressLayout->addWidget(logButton);
        mainLayout->addLayout(progressLayout);

        // Log area
        logArea = new QTextEdit;
        logArea->setReadOnly(true);
        logArea->setFont(QFont("Monospace", 10));
        logArea->setStyleSheet("background-color: #252525; color: #00ffff;");
        logArea->setVisible(false);
        mainLayout->addWidget(logArea);

        // Buttons
        QHBoxLayout *buttonLayout = new QHBoxLayout;
        QPushButton *configButton = new QPushButton("Configure Installation");
        QPushButton *mirrorButton = new QPushButton("Find Fastest Mirrors");
        QPushButton *installButton = new QPushButton("Start Installation");
        QPushButton *exitButton = new QPushButton("Exit");

        connect(configButton, &QPushButton::clicked, this, &CachyOSInstaller::configureInstallation);
        connect(mirrorButton, &QPushButton::clicked, this, &CachyOSInstaller::findFastestMirrors);
        connect(installButton, &QPushButton::clicked, this, &CachyOSInstaller::startInstallation);
        connect(exitButton, &QPushButton::clicked, qApp, &QApplication::quit);

        buttonLayout->addWidget(configButton);
        buttonLayout->addWidget(mirrorButton);
        buttonLayout->addWidget(installButton);
        buttonLayout->addWidget(exitButton);
        mainLayout->addLayout(buttonLayout);

        setCentralWidget(centralWidget);
        setWindowTitle("CachyOS BTRFS Installer");
        resize(800, 600);

        // Initialize empty settings
        initSettings();

        // Setup command runner thread
        commandThread = new QThread;
        commandRunner = new CommandRunner;
        commandRunner->moveToThread(commandThread);

        connect(this, &CachyOSInstaller::executeCommand, commandRunner, &CommandRunner::runCommand);
        connect(commandRunner, &CommandRunner::commandStarted, this, &CachyOSInstaller::logCommand);
        connect(commandRunner, &CommandRunner::commandOutput, this, &CachyOSInstaller::logOutput);
        connect(commandRunner, &CommandRunner::commandFinished, this, &CachyOSInstaller::commandCompleted);

        commandThread->start();
    }

    ~CachyOSInstaller() {
        commandThread->quit();
        commandThread->wait();
        delete commandRunner;
        delete commandThread;
    }

signals:
    void executeCommand(const QString &command, const QStringList &args = QStringList(), bool asRoot = false);

private slots:
    void toggleLog() {
        logArea->setVisible(!logArea->isVisible());
    }

    void configureInstallation() {
        QDialog dialog(this);
        dialog.setWindowTitle("Configure Installation");

        QFormLayout *form = new QFormLayout(&dialog);

        // Disk selection
        QLineEdit *diskEdit = new QLineEdit(settings["targetDisk"]);
        diskEdit->setPlaceholderText("e.g. /dev/sda");
        form->addRow("Target Disk:", diskEdit);

        // Hostname
        QLineEdit *hostnameEdit = new QLineEdit(settings["hostname"]);
        hostnameEdit->setPlaceholderText("e.g. cachyos");
        form->addRow("Hostname:", hostnameEdit);

        // Timezone
        QLineEdit *timezoneEdit = new QLineEdit(settings["timezone"]);
        timezoneEdit->setPlaceholderText("e.g. America/New_York");
        form->addRow("Timezone:", timezoneEdit);

        // Keymap
        QLineEdit *keymapEdit = new QLineEdit(settings["keymap"]);
        keymapEdit->setPlaceholderText("e.g. us");
        form->addRow("Keymap:", keymapEdit);

        // Username
        QLineEdit *userEdit = new QLineEdit(settings["username"]);
        userEdit->setPlaceholderText("e.g. user");
        form->addRow("Username:", userEdit);

        // Desktop environment
        QComboBox *desktopCombo = new QComboBox;
        desktopCombo->addItem("KDE Plasma", "KDE Plasma");
        desktopCombo->addItem("GNOME", "GNOME");
        desktopCombo->addItem("XFCE", "XFCE");
        desktopCombo->addItem("MATE", "MATE");
        desktopCombo->addItem("LXQt", "LXQt");
        desktopCombo->addItem("Cinnamon", "Cinnamon");
        desktopCombo->addItem("Budgie", "Budgie");
        desktopCombo->addItem("Deepin", "Deepin");
        desktopCombo->addItem("i3", "i3");
        desktopCombo->addItem("Sway", "Sway");
        desktopCombo->addItem("Hyprland", "Hyprland");
        desktopCombo->addItem("None", "None");
        if (!settings["desktopEnv"].isEmpty()) {
            desktopCombo->setCurrentText(settings["desktopEnv"]);
        }
        form->addRow("Desktop Environment:", desktopCombo);

        // Kernel selection
        QComboBox *kernelCombo = new QComboBox;
        kernelCombo->addItem("Bore", "linux-cachyos-bore");
        kernelCombo->addItem("Bore-Extra", "linux-cachyos-bore-extra");
        kernelCombo->addItem("CachyOS", "linux-cachyos");
        kernelCombo->addItem("CachyOS-Extra", "linux-cachyos-extra");
        kernelCombo->addItem("LTS", "linux-lts");
        kernelCombo->addItem("Zen", "linux-zen");
        if (!settings["kernel"].isEmpty()) {
            kernelCombo->setCurrentText(settings["kernel"]);
        }
        form->addRow("Kernel:", kernelCombo);

        // Bootloader
        QComboBox *bootloaderCombo = new QComboBox;
        bootloaderCombo->addItem("GRUB", "GRUB");
        bootloaderCombo->addItem("systemd-boot", "systemd-boot");
        bootloaderCombo->addItem("rEFInd", "rEFInd");
        if (!settings["bootloader"].isEmpty()) {
            bootloaderCombo->setCurrentText(settings["bootloader"]);
        }
        form->addRow("Bootloader:", bootloaderCombo);

        // Initramfs
        QComboBox *initramfsCombo = new QComboBox;
        initramfsCombo->addItem("mkinitcpio", "mkinitcpio");
        initramfsCombo->addItem("dracut", "dracut");
        initramfsCombo->addItem("booster", "booster");
        initramfsCombo->addItem("mkinitcpio-pico", "mkinitcpio-pico");
        if (!settings["initramfs"].isEmpty()) {
            initramfsCombo->setCurrentText(settings["initramfs"]);
        }
        form->addRow("Initramfs:", initramfsCombo);

        // Compression level
        QSpinBox *compressionSpin = new QSpinBox;
        compressionSpin->setRange(0, 22);
        if (!settings["compressionLevel"].isEmpty()) {
            compressionSpin->setValue(settings["compressionLevel"].toInt());
        } else {
            compressionSpin->setValue(3);
        }
        form->addRow("BTRFS Compression Level:", compressionSpin);

        // Gaming meta package option
        QCheckBox *gamingCheck = new QCheckBox("Install cachyos-gaming-meta");
        if (settings["gamingMeta"].toBool()) {
            gamingCheck->setChecked(true);
        }
        form->addRow(gamingCheck);

        // Repository selection
        QGroupBox *repoGroup = new QGroupBox("Additional Repositories");
        QVBoxLayout *repoLayout = new QVBoxLayout;
        
        QCheckBox *multilibCheck = new QCheckBox("Multilib");
        QCheckBox *testingCheck = new QCheckBox("Testing");
        QCheckBox *communityTestingCheck = new QCheckBox("Community Testing");
        QCheckBox *cachyosCheck = new QCheckBox("CachyOS");
        QCheckBox *cachyosV3Check = new QCheckBox("CachyOS V3");
        QCheckBox *cachyosTestingCheck = new QCheckBox("CachyOS Testing");
        
        if (settings["repos"].contains("multilib")) multilibCheck->setChecked(true);
        if (settings["repos"].contains("testing")) testingCheck->setChecked(true);
        if (settings["repos"].contains("community-testing")) communityTestingCheck->setChecked(true);
        if (settings["repos"].contains("cachyos")) cachyosCheck->setChecked(true);
        if (settings["repos"].contains("cachyos-v3")) cachyosV3Check->setChecked(true);
        if (settings["repos"].contains("cachyos-testing")) cachyosTestingCheck->setChecked(true);
        
        repoLayout->addWidget(multilibCheck);
        repoLayout->addWidget(testingCheck);
        repoLayout->addWidget(communityTestingCheck);
        repoLayout->addWidget(cachyosCheck);
        repoLayout->addWidget(cachyosV3Check);
        repoLayout->addWidget(cachyosTestingCheck);
        repoGroup->setLayout(repoLayout);
        form->addRow(repoGroup);

        // Password buttons
        QPushButton *rootPassButton = new QPushButton(settings["rootPassword"].isEmpty() ? "Set Root Password" : "Change Root Password");
        QPushButton *userPassButton = new QPushButton(settings["userPassword"].isEmpty() ? "Set User Password" : "Change User Password");
        form->addRow(rootPassButton);
        form->addRow(userPassButton);

        connect(rootPassButton, &QPushButton::clicked, [this, rootPassButton]() {
            PasswordDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
                settings["rootPassword"] = dlg.password();
                rootPassButton->setText("Change Root Password");
            }
        });

        connect(userPassButton, &QPushButton::clicked, [this, userPassButton]() {
            PasswordDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
                settings["userPassword"] = dlg.password();
                userPassButton->setText("Change User Password");
            }
        });

        QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                   Qt::Horizontal, &dialog);
        form->addRow(&buttonBox);

        connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if (dialog.exec() == QDialog::Accepted) {
            settings["targetDisk"] = diskEdit->text();
            settings["hostname"] = hostnameEdit->text();
            settings["timezone"] = timezoneEdit->text();
            settings["keymap"] = keymapEdit->text();
            settings["username"] = userEdit->text();
            settings["desktopEnv"] = desktopCombo->currentText();
            settings["kernel"] = kernelCombo->currentData().toString();
            settings["bootloader"] = bootloaderCombo->currentText();
            settings["initramfs"] = initramfsCombo->currentText();
            settings["compressionLevel"] = QString::number(compressionSpin->value());
            settings["gamingMeta"] = gamingCheck->isChecked();
            
            // Save repository selections
            settings["repos"].clear();
            if (multilibCheck->isChecked()) settings["repos"] << "multilib";
            if (testingCheck->isChecked()) settings["repos"] << "testing";
            if (communityTestingCheck->isChecked()) settings["repos"] << "community-testing";
            if (cachyosCheck->isChecked()) settings["repos"] << "cachyos";
            if (cachyosV3Check->isChecked()) settings["repos"] << "cachyos-v3";
            if (cachyosTestingCheck->isChecked()) settings["repos"] << "cachyos-testing";

            logMessage("Installation configured with the following settings:");
            logMessage(QString("Target Disk: %1").arg(settings["targetDisk"]));
            logMessage(QString("Hostname: %1").arg(settings["hostname"]));
            logMessage(QString("Timezone: %1").arg(settings["timezone"]));
            logMessage(QString("Keymap: %1").arg(settings["keymap"]));
            logMessage(QString("Username: %1").arg(settings["username"]));
            logMessage(QString("Desktop Environment: %1").arg(settings["desktopEnv"]));
            logMessage(QString("Kernel: %1").arg(settings["kernel"]));
            logMessage(QString("Bootloader: %1").arg(settings["bootloader"]));
            logMessage(QString("Initramfs: %1").arg(settings["initramfs"]));
            logMessage(QString("Compression Level: %1").arg(settings["compressionLevel"]));
            logMessage(QString("Gaming Meta: %1").arg(settings["gamingMeta"].toBool() ? "Yes" : "No"));
            logMessage(QString("Repositories: %1").arg(settings["repos"].join(", ")));
        }
    }

    void findFastestMirrors() {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Fastest Mirrors",
                                      "Would you like to find and use the fastest mirrors?",
                                      QMessageBox::Yes|QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            logMessage("Finding fastest mirrors...");
            progressBar->setValue(10);

            // Install reflector if not present
            emit executeCommand("pacman", {"-Sy", "--noconfirm", "reflector"}, true);

            // Find and update mirrors
            emit executeCommand("reflector", {"--latest", "20", "--protocol", "https", "--sort", "rate", "--save", "/etc/pacman.d/mirrorlist"}, true);

            progressBar->setValue(100);
            QTimer::singleShot(1000, [this]() {
                progressBar->setValue(0);
            });
        } else {
            logMessage("Using default mirrors");
        }
    }

    void startInstallation() {
        // Validate all required settings
        QStringList missingFields;
        if (settings["targetDisk"].isEmpty()) missingFields << "Target Disk";
        if (settings["hostname"].isEmpty()) missingFields << "Hostname";
        if (settings["timezone"].isEmpty()) missingFields << "Timezone";
        if (settings["keymap"].isEmpty()) missingFields << "Keymap";
        if (settings["username"].isEmpty()) missingFields << "Username";
        if (settings["desktopEnv"].isEmpty()) missingFields << "Desktop Environment";
        if (settings["kernel"].isEmpty()) missingFields << "Kernel";
        if (settings["bootloader"].isEmpty()) missingFields << "Bootloader";
        if (settings["initramfs"].isEmpty()) missingFields << "Initramfs";
        if (settings["compressionLevel"].isEmpty()) missingFields << "Compression Level";
        if (settings["rootPassword"].isEmpty()) missingFields << "Root Password";
        if (settings["userPassword"].isEmpty()) missingFields << "User Password";

        if (!missingFields.isEmpty()) {
            QMessageBox::warning(this, "Error",
                                 QString("The following required fields are missing:\n%1\n\nPlease configure all settings before installation.")
                                 .arg(missingFields.join("\n")));
            return;
        }

        // Show confirmation dialog
        QString confirmationText = QString(
            "About to install to %1 with these settings:\n"
            "Hostname: %2\n"
            "Timezone: %3\n"
            "Keymap: %4\n"
            "Username: %5\n"
            "Desktop: %6\n"
            "Kernel: %7\n"
            "Bootloader: %8\n"
            "Initramfs: %9\n"
            "Compression Level: %10\n"
            "Gaming Meta: %11\n"
            "Repositories: %12\n\n"
            "Continue?"
        ).arg(
            settings["targetDisk"],
            settings["hostname"],
            settings["timezone"],
            settings["keymap"],
            settings["username"],
            settings["desktopEnv"],
            settings["kernel"],
            settings["bootloader"],
            settings["initramfs"],
            settings["compressionLevel"],
            settings["gamingMeta"].toBool() ? "Yes" : "No",
            settings["repos"].join(", ")
        );

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Confirm Installation", confirmationText,
                                      QMessageBox::Yes|QMessageBox::No);

        if (reply != QMessageBox::Yes) {
            logMessage("Installation cancelled.");
            return;
        }

        // Ask for sudo password before proceeding
        PasswordDialog passDialog(this);
        passDialog.setWindowTitle("Enter sudo Password");
        if (passDialog.exec() != QDialog::Accepted) {
            logMessage("Installation cancelled - no password provided.");
            return;
        }

        // Set sudo password for command runner
        commandRunner->setSudoPassword(passDialog.password());

        // Start installation process
        logMessage("Starting CachyOS BTRFS installation...");
        progressBar->setValue(5);

        // Begin installation steps
        currentStep = 0;
        totalSteps = 15; // Total number of installation steps
        nextInstallationStep();
    }

    void nextInstallationStep() {
        currentStep++;
        int progress = (currentStep * 100) / totalSteps;
        progressBar->setValue(progress);

        QString disk1 = settings["targetDisk"] + "1";
        QString disk2 = settings["targetDisk"] + "2";
        QString compression = "zstd:" + settings["compressionLevel"];

        switch (currentStep) {
            case 1: // Install required tools
                logMessage("Installing required tools...");
                emit executeCommand("pacman", {"-Sy", "--noconfirm", "btrfs-progs", "parted", "dosfstools", "efibootmgr"}, true);
                break;

            case 2: // Partitioning
                logMessage("Partitioning disk...");
                emit executeCommand("parted", {"-s", settings["targetDisk"], "mklabel", "gpt"}, true);
                emit executeCommand("parted", {"-s", settings["targetDisk"], "mkpart", "primary", "1MiB", "513MiB"}, true);
                emit executeCommand("parted", {"-s", settings["targetDisk"], "set", "1", "esp", "on"}, true);
                emit executeCommand("parted", {"-s", settings["targetDisk"], "mkpart", "primary", "513MiB", "100%"}, true);
                break;

            case 3: // Formatting
                logMessage("Formatting partitions...");
                emit executeCommand("mkfs.vfat", {"-F32", disk1}, true);
                emit executeCommand("mkfs.btrfs", {"-f", disk2}, true);
                break;

            case 4: // Mounting and subvolumes
                logMessage("Creating BTRFS subvolumes...");
                emit executeCommand("mount", {disk2, "/mnt"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@home"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@root"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@srv"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@tmp"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@log"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@cache"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@/var/lib/portables"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@/var/lib/machines"}, true);
                emit executeCommand("umount", {"/mnt"}, true);
                break;

            case 5: // Remount with compression
                logMessage("Remounting with compression...");
                emit executeCommand("mount", {"-o", QString("subvol=@,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/boot/efi"}, true);
                emit executeCommand("mount", {disk1, "/mnt/boot/efi"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/home"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/root"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/srv"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/tmp"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/var/cache"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/var/log"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/var/lib/portables"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/var/lib/machines"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@home,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/home"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@root,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/root"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@srv,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/srv"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@tmp,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/tmp"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@log,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/var/log"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@cache,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/var/cache"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@/var/lib/portables,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/var/lib/portables"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@/var/lib/machines,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/var/lib/machines"}, true);
                break;

            case 6: // Install base system with pacstrap
                logMessage("Installing base system with pacstrap...");
                {
                    QStringList basePkgs = {"base", settings["kernel"], "linux-firmware", "btrfs-progs", "nano"};
                    
                    // Add bootloader packages
                    if (settings["bootloader"] == "GRUB") {
                        basePkgs << "grub" << "efibootmgr" << "dosfstools" << "cachyos-grub-theme";
                    } else if (settings["bootloader"] == "systemd-boot") {
                        basePkgs << "efibootmgr";
                    } else if (settings["bootloader"] == "rEFInd") {
                        basePkgs << "refind";
                    }
                    
                    // Add initramfs packages
                    basePkgs << settings["initramfs"];
                    
                    // Only add network manager if no desktop selected
                    if (settings["desktopEnv"] == "None") {
                        basePkgs << "networkmanager";
                    }

                    // Add gaming meta if selected
                    if (settings["gamingMeta"].toBool()) {
                        basePkgs << "cachyos-gaming-meta";
                    }

                    // Build the pacstrap command
                    QStringList pacstrapArgs;
                    pacstrapArgs << "-i" << "/mnt";
                    pacstrapArgs.append(basePkgs);
                    
                    emit executeCommand("pacstrap", pacstrapArgs, true);
                }
                break;

            case 7: // Enable selected repositories
                logMessage("Enabling selected repositories...");
                for (const QString &repo : settings["repos"]) {
                    if (repo == "cachyos" || repo == "cachyos-v3" || repo == "cachyos-testing") {
                        // Import CachyOS key
                        emit executeCommand("arch-chroot", {"/mnt", "pacman-key", "--recv-keys", "F3B607488DB35A47", "--keyserver", "keyserver.ubuntu.com"}, true);
                        emit executeCommand("arch-chroot", {"/mnt", "pacman-key", "--lsign-key", "F3B607488DB35A47"}, true);
                    }
                }
                break;

            case 8: // Generate fstab
                logMessage("Generating fstab...");
                {
                    QString rootUuid = getDiskUuid(disk2);
                    QString fstabContent = QString(
                        "# Btrfs subvolumes (auto-added)\n"
                        "UUID=%1 /              btrfs   rw,noatime,compress=%2,discard=async,space_cache=v2,subvol=/@ 0 0\n"
                        "UUID=%1 /root          btrfs   rw,noatime,compress=%2,discard=async,space_cache=v2,subvol=/@root 0 0\n"
                        "UUID=%1 /home          btrfs   rw,noatime,compress=%2,discard=async,space_cache=v2,subvol=/@home 0 0\n"
                        "UUID=%1 /srv           btrfs   rw,noatime,compress=%2,discard=async,space_cache=v2,subvol=/@srv 0 0\n"
                        "UUID=%1 /var/cache     btrfs   rw,noatime,compress=%2,discard=async,space_cache=v2,subvol=/@cache 0 0\n"
                        "UUID=%1 /var/tmp       btrfs   rw,noatime,compress=%2,discard=async,space_cache=v2,subvol=/@tmp 0 0\n"
                        "UUID=%1 /var/log       btrfs   rw,noatime,compress=%2,discard=async,space_cache=v2,subvol=/@log 0 0\n"
                        "UUID=%1 /var/lib/portables btrfs rw,noatime,compress=%2,discard=async,space_cache=v2,subvol=/@/var/lib/portables 0 0\n"
                        "UUID=%1 /var/lib/machines btrfs rw,noatime,compress=%2,discard=async,space_cache=v2,subvol=/@/var/lib/machines 0 0\n"
                    ).arg(rootUuid, compression);
                    
                    QTemporaryFile tempFile;
                    if (tempFile.open()) {
                        QTextStream out(&tempFile);
                        out << fstabContent;
                        tempFile.close();
                        emit executeCommand("cat", {tempFile.fileName(), ">>", "/mnt/etc/fstab"}, true);
                    }
                }
                break;

            case 9: // Mount required filesystems for chroot
                logMessage("Preparing chroot environment...");
                emit executeCommand("mount", {"-t", "proc", "none", "/mnt/proc"}, true);
                emit executeCommand("mount", {"--rbind", "/dev", "/mnt/dev"}, true);
                emit executeCommand("mount", {"--rbind", "/sys", "/mnt/sys"}, true);
                break;

            case 10: // Create chroot setup script
                logMessage("Preparing chroot setup script...");
                createChrootScript();
                emit executeCommand("chmod", {"+x", "/mnt/setup-chroot.sh"}, true);
                break;

            case 11: // Run chroot setup
                logMessage("Running chroot setup...");
                emit executeCommand("arch-chroot", {"/mnt", "/setup-chroot.sh"}, true);
                break;

            case 12: // Clean up
                logMessage("Cleaning up...");
                emit executeCommand("umount", {"-R", "/mnt"}, true);
                break;

            case 13: // Installation complete
                logMessage("Installation complete!");
                progressBar->setValue(100);
                showPostInstallOptions();
                break;

            default:
                break;
        }
    }

    void commandCompleted(bool success) {
        if (!success) {
            logMessage("ERROR: Command failed!");
            QMessageBox::critical(this, "Error", "A command failed during installation. Check the log for details.");
            progressBar->setValue(0);
            return;
        }

        // Proceed to next step
        QTimer::singleShot(500, this, &CachyOSInstaller::nextInstallationStep);
    }

    void createChrootScript() {
        QTemporaryFile tempFile;
        if (tempFile.open()) {
            QTextStream out(&tempFile);

            out << "#!/bin/bash\n\n";
            out << "# Basic system configuration\n";
            out << "echo \"" << settings["hostname"] << "\" > /etc/hostname\n";
            out << "ln -sf /usr/share/zoneinfo/" << settings["timezone"] << " /etc/localtime\n";
            out << "hwclock --systohc\n";
            out << "echo \"en_US.UTF-8 UTF-8\" >> /etc/locale.gen\n";
            out << "locale-gen\n";
            out << "echo \"LANG=en_US.UTF-8\" > /etc/locale.conf\n";
            out << "echo \"KEYMAP=" << settings["keymap"] << "\" > /etc/vconsole.conf\n\n";

            out << "# Users and passwords\n";
            out << "echo \"root:" << settings["rootPassword"] << "\" | chpasswd\n";
            out << "useradd -m -G wheel,audio,video,storage,optical -s /bin/bash " << settings["username"] << "\n";
            out << "echo \"" << settings["username"] << ":" << settings["userPassword"] << "\" | chpasswd\n\n";

            out << "# Configure sudo\n";
            out << "echo \"%wheel ALL=(ALL) ALL\" > /etc/sudoers.d/wheel\n\n";

            // Handle bootloader installation
            out << "# Install bootloader\n";
            if (settings["bootloader"] == "GRUB") {
                out << "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=CachyOS\n";
                out << "grub-mkconfig -o /boot/grub/grub.cfg\n";
            } else if (settings["bootloader"] == "systemd-boot") {
                out << "bootctl --path=/boot/efi install\n";
                out << "mkdir -p /boot/efi/loader/entries\n";
                out << "cat > /boot/efi/loader/loader.conf << 'LOADER'\n";
                out << "default arch\n";
                out << "timeout 3\n";
                out << "editor  yes\n";
                out << "LOADER\n\n";

                out << "cat > /boot/efi/loader/entries/arch.conf << 'ENTRY'\n";
                out << "title   CachyOS Linux\n";
                out << "linux   /vmlinuz-" << settings["kernel"] << "\n";
                out << "initrd  /initramfs-" << settings["kernel"] << ".img\n";
                out << "options root=UUID=" << getDiskUuid(settings["targetDisk"] + "2") << " rootflags=subvol=@ rw\n";
                out << "ENTRY\n";
            } else if (settings["bootloader"] == "rEFInd") {
                out << "refind-install\n";
                out << "mkdir -p /boot/efi/EFI/refind/refind.conf\n";
                out << "cat > /boot/efi/EFI/refind/refind.conf << 'REFIND'\n";
                out << "menuentry \"CachyOS Linux\" {\n";
                out << "    icon     /EFI/refind/icons/os_arch.png\n";
                out << "    loader   /vmlinuz-" << settings["kernel"] << "\n";
                out << "    initrd   /initramfs-" << settings["kernel"] << ".img\n";
                out << "    options  \"root=UUID=" << getDiskUuid(settings["targetDisk"] + "2") << " rootflags=subvol=@ rw\"\n";
                out << "}\n";
                out << "REFIND\n";
            }

            // Handle initramfs
            out << "\n# Generate initramfs\n";
            if (settings["initramfs"] == "mkinitcpio") {
                out << "mkinitcpio -P\n";
            } else if (settings["initramfs"] == "dracut") {
                out << "dracut --regenerate-all --force\n";
            } else if (settings["initramfs"] == "booster") {
                out << "booster generate\n";
            } else if (settings["initramfs"] == "mkinitcpio-pico") {
                out << "mkinitcpio -P\n";
            }

            // Network manager (only enable if no desktop selected)
            if (settings["desktopEnv"] == "None") {
                out << "\n# Enable NetworkManager\n";
                out << "systemctl enable NetworkManager\n";
            }

            // Install desktop environment and related packages
            out << "\n# Install desktop environment\n";
            if (settings["desktopEnv"] == "KDE Plasma") {
                out << "pacman -S --noconfirm plasma-meta kde-applications-meta sddm cachyos-kde-settings\n";
                out << "systemctl enable sddm\n";
                out << "pacman -S --noconfirm firefox dolphin konsole\n";
            } else if (settings["desktopEnv"] == "GNOME") {
                out << "pacman -S --noconfirm gnome gnome-extra gdm\n";
                out << "systemctl enable gdm\n";
                out << "pacman -S --noconfirm firefox gnome-terminal\n";
            } else if (settings["desktopEnv"] == "XFCE") {
                out << "pacman -S --noconfirm xfce4 xfce4-goodies lightdm lightdm-gtk-greeter\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox mousepad xfce4-terminal\n";
            } else if (settings["desktopEnv"] == "MATE") {
                out << "pacman -S --noconfirm mate mate-extra mate-media lightdm lightdm-gtk-greeter\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox pluma mate-terminal\n";
            } else if (settings["desktopEnv"] == "LXQt") {
                out << "pacman -S --noconfirm lxqt breeze-icons sddm\n";
                out << "systemctl enable sddm\n";
                out << "pacman -S --noconfirm firefox qterminal\n";
            } else if (settings["desktopEnv"] == "Cinnamon") {
                out << "pacman -S --noconfirm cinnamon cinnamon-translations lightdm lightdm-gtk-greeter\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox xed gnome-terminal\n";
            } else if (settings["desktopEnv"] == "Budgie") {
                out << "pacman -S --noconfirm budgie-desktop budgie-extras gnome-control-center gnome-terminal lightdm lightdm-gtk-greeter\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox gnome-text-editor gnome-terminal\n";
            } else if (settings["desktopEnv"] == "Deepin") {
                out << "pacman -S --noconfirm deepin deepin-extra lightdm\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox deepin-terminal\n";
            } else if (settings["desktopEnv"] == "i3") {
                out << "pacman -S --noconfirm i3-wm i3status i3lock dmenu lightdm lightdm-gtk-greeter\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox alacritty\n";
            } else if (settings["desktopEnv"] == "Sway") {
                out << "pacman -S --noconfirm sway swaylock swayidle waybar wofi lightdm lightdm-gtk-greeter\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox foot\n";
            } else if (settings["desktopEnv"] == "Hyprland") {
                out << "pacman -S --noconfirm hyprland waybar rofi wofi kitty swaybg swaylock-effects wl-clipboard lightdm lightdm-gtk-greeter\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox kitty\n";
                
                out << "# Create Hyprland config directory\n";
                out << "mkdir -p /home/" << settings["username"] << "/.config/hypr\n";
                out << "cat > /home/" << settings["username"] << "/.config/hypr/hyprland.conf << 'HYPRCONFIG'\n";
                out << "# This is a basic Hyprland config\n";
                out << "exec-once = waybar &\n";
                out << "exec-once = swaybg -i ~/wallpaper.jpg &\n\n";
                out << "monitor=,preferred,auto,1\n\n";
                out << "input {\n";
                out << "    kb_layout = us\n";
                out << "    follow_mouse = 1\n";
                out << "    touchpad {\n";
                out << "        natural_scroll = yes\n";
                out << "    }\n";
                out << "}\n\n";
                out << "general {\n";
                out << "    gaps_in = 5\n";
                out << "    gaps_out = 10\n";
                out << "    border_size = 2\n";
                out << "    col.active_border = rgba(33ccffee) rgba(00ff99ee) 45deg\n";
                out << "    col.inactive_border = rgba(595959aa)\n";
                out << "}\n\n";
                out << "decoration {\n";
                out << "    rounding = 5\n";
                out << "    blur = yes\n";
                out << "    blur_size = 3\n";
                out << "    blur_passes = 1\n";
                out << "    blur_new_optimizations = on\n";
                out << "}\n\n";
                out << "animations {\n";
                out << "    enabled = yes\n";
                out << "    bezier = myBezier, 0.05, 0.9, 0.1, 1.05\n";
                out << "    animation = windows, 1, 7, myBezier\n";
                out << "    animation = windowsOut, 1, 7, default, popin 80%\n";
                out << "    animation = border, 1, 10, default\n";
                out << "    animation = fade, 1, 7, default\n";
                out << "    animation = workspaces, 1, 6, default\n";
                out << "}\n\n";
                out << "dwindle {\n";
                out << "    pseudotile = yes\n";
                out << "    preserve_split = yes\n";
                out << "}\n\n";
                out << "master {\n";
                out << "    new_is_master = true\n";
                out << "}\n\n";
                out << "bind = SUPER, Return, exec, kitty\n";
                out << "bind = SUPER, Q, killactive,\n";
                out << "bind = SUPER, M, exit,\n";
                out << "bind = SUPER, V, togglefloating,\n";
                out << "bind = SUPER, F, fullscreen,\n";
                out << "bind = SUPER, D, exec, rofi -show drun\n";
                out << "bind = SUPER, P, pseudo,\n";
                out << "bind = SUPER, J, togglesplit,\n";
                out << "HYPRCONFIG\n";
                
                out << "# Set ownership of config files\n";
                out << "chown -R " << settings["username"] << ":" << settings["username"] << " /home/" << settings["username"] << "/.config\n";
            }

            // Install gaming meta if selected
            if (settings["gamingMeta"].toBool()) {
                out << "\n# Install gaming packages\n";
                out << "pacman -S --noconfirm cachyos-gaming-meta\n";
            }

            // Enable TRIM for SSDs
            out << "\n# Enable TRIM\n";
            out << "systemctl enable fstrim.timer\n";

            // Clean up
            out << "\n# Clean up\n";
            out << "rm /setup-chroot.sh\n";

            tempFile.close();

            // Copy the temporary file to /mnt/setup-chroot.sh
            QProcess::execute("cp", {tempFile.fileName(), "/mnt/setup-chroot.sh"});
        }
    }

    void showPostInstallOptions() {
        QDialog dialog(this);
        dialog.setWindowTitle("Installation Complete");

        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        QLabel *label = new QLabel("Installation complete! Select post-install action:");
        layout->addWidget(label);

        QPushButton *rebootButton = new QPushButton("Reboot now");
        QPushButton *chrootButton = new QPushButton("Chroot into installed system");
        QPushButton *exitButton = new QPushButton("Exit without rebooting");

        layout->addWidget(rebootButton);
        layout->addWidget(chrootButton);
        layout->addWidget(exitButton);

        connect(rebootButton, &QPushButton::clicked, [this, &dialog]() {
            QProcess::execute("reboot", QStringList());
            dialog.accept();
        });

        connect(chrootButton, &QPushButton::clicked, [this, &dialog]() {
            logMessage("Entering chroot...");
            QString disk1 = settings["targetDisk"] + "1";
            QString disk2 = settings["targetDisk"] + "2";

            emit executeCommand("mount", {disk1, "/mnt/boot/efi"}, true);
            emit executeCommand("mount", {"-o", "subvol=@", disk2, "/mnt"}, true);
            emit executeCommand("mount", {"-t", "proc", "none", "/mnt/proc"}, true);
            emit executeCommand("mount", {"--rbind", "/dev", "/mnt/dev"}, true);
            emit executeCommand("mount", {"--rbind", "/sys", "/mnt/sys"}, true);
            emit executeCommand("mount", {"--rbind", "/dev/pts", "/mnt/dev/pts"}, true);
            emit executeCommand("arch-chroot", {"/mnt", "/bin/bash"}, true);
            emit executeCommand("umount", {"-l", "/mnt"}, true);

            dialog.accept();
        });

        connect(exitButton, &QPushButton::clicked, &dialog, &QDialog::accept);

        dialog.exec();
    }

    void logMessage(const QString &message) {
        logArea->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss"), message));
        logArea->verticalScrollBar()->setValue(logArea->verticalScrollBar()->maximum());
    }

    void logCommand(const QString &command) {
        logMessage("Executing: " + command);
    }

    void logOutput(const QString &output) {
        logArea->insertPlainText(output);
        logArea->verticalScrollBar()->setValue(logArea->verticalScrollBar()->maximum());
    }

    QString getDiskUuid(const QString &disk) {
        QProcess process;
        process.start("blkid", {"-s", "UUID", "-o", "value", disk});
        process.waitForFinished();
        return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    }

private:
    void initSettings() {
        settings["targetDisk"] = "";
        settings["hostname"] = "";
        settings["timezone"] = "";
        settings["keymap"] = "";
        settings["username"] = "";
        settings["desktopEnv"] = "";
        settings["kernel"] = "";
        settings["bootloader"] = "";
        settings["initramfs"] = "";
        settings["compressionLevel"] = "";
        settings["rootPassword"] = "";
        settings["userPassword"] = "";
        settings["gamingMeta"] = false;
        settings["repos"] = QStringList();
    }

    QProgressBar *progressBar;
    QTextEdit *logArea;
    QMap<QString, QVariant> settings;
    int currentStep;
    int totalSteps;
    CommandRunner *commandRunner;
    QThread *commandThread;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Check if running as root
    if (QProcess::execute("whoami", QStringList()) != 0) {
        QMessageBox::critical(nullptr, "Error", "This application must be run as root!");
        return 1;
    }

    CachyOSInstaller installer;
    installer.show();

    return app.exec();
}

#include "main.moc"
