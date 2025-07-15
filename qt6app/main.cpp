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
    explicit CommandRunner(QObject *parent = nullptr) : QObject(parent), m_currentProgress(0) {}
    QString password() const { return m_sudoPassword; }

signals:
    void commandStarted(const QString &command);
    void commandOutput(const QString &output);
    void commandFinished(bool success);
    void progressUpdate(int value);

public slots:
    void runCommand(const QString &command, const QStringList &args = QStringList(), bool asRoot = false, int progressWeight = 1) {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        QString fullCommand;
        if (asRoot) {
            fullCommand = "sudo -S " + command;
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
            fullArgs << "-S" << command;
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
            m_currentProgress += progressWeight;
            emit progressUpdate(m_currentProgress);
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

    void resetProgress() {
        m_currentProgress = 0;
    }

private:
    QString m_sudoPassword;
    int m_currentProgress;
};

class CachyOSInstaller : public QMainWindow {
    Q_OBJECT

public:
    CachyOSInstaller(QWidget *parent = nullptr) : QMainWindow(parent) {
        // Set dark theme
        qApp->setStyle(QStyleFactory::create("Fusion"));
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
        darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(darkPalette);

        // Create main widgets
        QWidget *centralWidget = new QWidget;
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        // ASCII art title
        QLabel *titleLabel = new QLabel;
        titleLabel->setText(
            "<pre><span style='color:#ff0000;'>"
            "░█████╗░██╗░░░░░░█████╗░██╗░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗<br>"
            "██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝<br>"
            "██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░<br>"
            "██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗<br>"
            "╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝<br>"
            "░╚════╝░╚══════╝╚═╝░░╚═╝░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░</span><br>"
            "<span style='color:#00ffff;'>CachyOS Btrfs Installer v1.1</span></pre>"
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

        // Hint label
        hintLabel = new QLabel;
        hintLabel->setStyleSheet("color: #00ffff; font-style: italic;");
        hintLabel->setWordWrap(true);
        hintLabel->setText("Hint: Select your installation options and click 'Configure Installation' to begin.");
        mainLayout->addWidget(hintLabel);

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
        QPushButton *mirrorButton = new QPushButton("Mirror Options");
        QPushButton *installButton = new QPushButton("Start Installation");
        QPushButton *exitButton = new QPushButton("Exit");

        connect(configButton, &QPushButton::clicked, this, &CachyOSInstaller::configureInstallation);
        connect(mirrorButton, &QPushButton::clicked, this, &CachyOSInstaller::showMirrorOptions);
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
        connect(commandRunner, &CommandRunner::progressUpdate, this, &CachyOSInstaller::updateProgress);
        commandThread->start();
    }

    ~CachyOSInstaller() {
        commandThread->quit();
        commandThread->wait();
        delete commandRunner;
        delete commandThread;
    }

signals:
    void executeCommand(const QString &command, const QStringList &args = QStringList(), bool asRoot = false, int progressWeight = 1);

private slots:
    void toggleLog() {
        logArea->setVisible(!logArea->isVisible());
    }

    void updateProgress(int value) {
        progressBar->setValue(value);
    }

    void configureInstallation() {
        QDialog dialog(this);
        dialog.setWindowTitle("Configure Installation");

        // Use horizontal layout for better alignment
        QHBoxLayout *mainLayout = new QHBoxLayout(&dialog);

        // Left column
        QFormLayout *leftForm = new QFormLayout;
        QLineEdit *diskEdit = new QLineEdit(settings["targetDisk"].toString());
        diskEdit->setPlaceholderText("e.g. /dev/sda");
        leftForm->addRow("Target Disk:", diskEdit);

        QPushButton *wipeButton = new QPushButton("Wipe Disk");
        connect(wipeButton, &QPushButton::clicked, [this, diskEdit]() {
            QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Wipe",
                                                                      "WARNING: This will erase ALL data on " + diskEdit->text() + "!\nAre you sure you want to continue?",
                                                                      QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                logMessage("Wiping disk " + diskEdit->text() + "...");
                emit executeCommand("wipefs", {"-a", diskEdit->text()}, true, 0);
                logMessage("Disk wiped successfully.");
            }
        });
        leftForm->addRow("Disk Operations:", wipeButton);

        QLineEdit *hostnameEdit = new QLineEdit(settings["hostname"].toString());
        hostnameEdit->setPlaceholderText("e.g. cachyos");
        leftForm->addRow("Hostname:", hostnameEdit);

        QLineEdit *timezoneEdit = new QLineEdit(settings["timezone"].toString());
        timezoneEdit->setPlaceholderText("e.g. America/New_York");
        leftForm->addRow("Timezone:", timezoneEdit);

        QLineEdit *keymapEdit = new QLineEdit(settings["keymap"].toString());
        keymapEdit->setPlaceholderText("e.g. us");
        leftForm->addRow("Keymap:", keymapEdit);

        QLineEdit *userEdit = new QLineEdit(settings["username"].toString());
        userEdit->setPlaceholderText("e.g. user");
        leftForm->addRow("Username:", userEdit);

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
        if (!settings["desktopEnv"].toString().isEmpty()) {
            desktopCombo->setCurrentText(settings["desktopEnv"].toString());
        }
        leftForm->addRow("Desktop Environment:", desktopCombo);

        mainLayout->addLayout(leftForm);

        // Right column
        QFormLayout *rightForm = new QFormLayout;

        QComboBox *kernelCombo = new QComboBox;
        kernelCombo->addItem("Bore", "linux-cachyos-bore");
        kernelCombo->addItem("Bore-Extra", "linux-cachyos-bore-extra");
        kernelCombo->addItem("CachyOS", "linux-cachyos");
        kernelCombo->addItem("CachyOS-Extra", "linux-cachyos-extra");
        kernelCombo->addItem("LTS", "linux-lts");
        kernelCombo->addItem("Zen", "linux-zen");
        if (!settings["kernel"].toString().isEmpty()) {
            kernelCombo->setCurrentText(settings["kernel"].toString());
        }
        rightForm->addRow("Kernel:", kernelCombo);

        QComboBox *bootloaderCombo = new QComboBox;
        bootloaderCombo->addItem("GRUB", "GRUB");
        bootloaderCombo->addItem("systemd-boot", "systemd-boot");
        bootloaderCombo->addItem("rEFInd", "rEFInd");
        if (!settings["bootloader"].toString().isEmpty()) {
            bootloaderCombo->setCurrentText(settings["bootloader"].toString());
        }
        rightForm->addRow("Bootloader:", bootloaderCombo);

        QComboBox *initramfsCombo = new QComboBox;
        initramfsCombo->addItem("mkinitcpio", "mkinitcpio");
        initramfsCombo->addItem("dracut", "dracut");
        initramfsCombo->addItem("booster", "booster");
        initramfsCombo->addItem("mkinitcpio-pico", "mkinitcpio-pico");
        if (!settings["initramfs"].toString().isEmpty()) {
            initramfsCombo->setCurrentText(settings["initramfs"].toString());
        }
        rightForm->addRow("Initramfs:", initramfsCombo);

        QSpinBox *compressionSpin = new QSpinBox;
        compressionSpin->setRange(0, 22);
        if (!settings["compressionLevel"].toString().isEmpty()) {
            compressionSpin->setValue(settings["compressionLevel"].toInt());
        } else {
            compressionSpin->setValue(3);
        }
        rightForm->addRow("BTRFS Compression Level:", compressionSpin);

        QCheckBox *gamingCheck = new QCheckBox("Install cachyos-gaming-meta");
        if (settings["gamingMeta"].toBool()) {
            gamingCheck->setChecked(true);
        }
        rightForm->addRow(gamingCheck);

        mainLayout->addLayout(rightForm);

        // Buttons for passwords
        QPushButton *rootPassButton = new QPushButton(settings["rootPassword"].toString().isEmpty() ? "Set Root Password" : "Change Root Password");
        QPushButton *userPassButton = new QPushButton(settings["userPassword"].toString().isEmpty() ? "Set User Password" : "Change User Password");

        QVBoxLayout *buttonLayout = new QVBoxLayout;
        buttonLayout->addWidget(rootPassButton);
        buttonLayout->addWidget(userPassButton);
        mainLayout->addLayout(buttonLayout);

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

        // Dialog buttons
        QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
        QHBoxLayout *buttonLayoutFinal = new QHBoxLayout;
        buttonLayoutFinal->addWidget(&buttonBox);
        mainLayout->addLayout(buttonLayoutFinal);

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

            logMessage("Installation configured with the following settings:");
            logMessage(QString("Target Disk: %1").arg(settings["targetDisk"].toString()));
            logMessage(QString("Hostname: %1").arg(settings["hostname"].toString()));
            logMessage(QString("Timezone: %1").arg(settings["timezone"].toString()));
            logMessage(QString("Keymap: %1").arg(settings["keymap"].toString()));
            logMessage(QString("Username: %1").arg(settings["username"].toString()));
            logMessage(QString("Desktop Environment: %1").arg(settings["desktopEnv"].toString()));
            logMessage(QString("Kernel: %1").arg(settings["kernel"].toString()));
            logMessage(QString("Bootloader: %1").arg(settings["bootloader"].toString()));
            logMessage(QString("Initramfs: %1").arg(settings["initramfs"].toString()));
            logMessage(QString("Compression Level: %1").arg(settings["compressionLevel"].toString()));
            logMessage(QString("Gaming Meta: %1").arg(settings["gamingMeta"].toBool() ? "Yes" : "No"));

            hintLabel->setText("<span style='color:#00ffff;'>Configuration saved. You can now start the installation or adjust mirror options.</span>");
        }
    }

    void showMirrorOptions() {
        QDialog dialog(this);
        dialog.setWindowTitle("Mirror Options");
        QVBoxLayout *layout = new QVBoxLayout(&dialog);

        QGroupBox *mirrorGroup = new QGroupBox("Mirror Selection");
        QVBoxLayout *mirrorLayout = new QVBoxLayout;

        QCheckBox *archMirrors = new QCheckBox("Use Arch Linux mirrors");
        archMirrors->setChecked(settings["useArchMirrors"].toBool());

        QCheckBox *cachyosMirrors = new QCheckBox("Use CachyOS mirrors");
        cachyosMirrors->setChecked(settings["useCachyosMirrors"].toBool());

        QCheckBox *fastestMirrors = new QCheckBox("Find and use fastest mirrors");
        fastestMirrors->setChecked(settings["useFastestMirrors"].toBool());

        QCheckBox *testingMirrors = new QCheckBox("Include testing repositories");
        testingMirrors->setChecked(settings["useTestingMirrors"].toBool());

        mirrorLayout->addWidget(archMirrors);
        mirrorLayout->addWidget(cachyosMirrors);
        mirrorLayout->addWidget(fastestMirrors);
        mirrorLayout->addWidget(testingMirrors);
        mirrorGroup->setLayout(mirrorLayout);
        layout->addWidget(mirrorGroup);

        QPushButton *applyButton = new QPushButton("Apply Mirror Configuration");
        connect(applyButton, &QPushButton::clicked, [&]() {
            settings["useArchMirrors"] = archMirrors->isChecked();
            settings["useCachyosMirrors"] = cachyosMirrors->isChecked();
            settings["useFastestMirrors"] = fastestMirrors->isChecked();
            settings["useTestingMirrors"] = testingMirrors->isChecked();

            if (fastestMirrors->isChecked()) {
                updateMirrors();
            } else {
                configureStaticMirrors();
            }

            dialog.accept();
        });

        QDialogButtonBox buttonBox(QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
        connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        layout->addWidget(applyButton);
        layout->addWidget(&buttonBox);

        dialog.exec();
    }

    void updateMirrors() {
        logMessage("Updating mirror list...");
        hintLabel->setText("<span style='color:#00ffff;'>Updating mirror list, this may take a few minutes...</span>");

        // Install reflector if not present
        emit executeCommand("pacman", {"-Sy", "--noconfirm", "reflector"}, true, 5);

        // Build reflector command
        QStringList reflectorArgs;
        reflectorArgs << "--latest" << "20" << "--protocol" << "https" << "--sort" << "rate";

        if (settings["useArchMirrors"].toBool()) {
            reflectorArgs << "--country" << "Global";
        }
        if (settings["useCachyosMirrors"].toBool()) {
            reflectorArgs << "--country" << "Germany";
        }

        reflectorArgs << "--save" << "/etc/pacman.d/mirrorlist";

        emit executeCommand("reflector", reflectorArgs, true, 10);

        if (settings["useTestingMirrors"].toBool()) {
            QFile mirrorlist("/etc/pacman.d/mirrorlist");
            if (mirrorlist.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream out(&mirrorlist);
                out << "\n[testing]\n";
                out << "Include = /etc/pacman.d/mirrorlist\n";
                mirrorlist.close();
            }
        }

        logMessage("Mirror list updated successfully.");
        hintLabel->setText("<span style='color:#00ffff;'>Mirror configuration updated successfully.</span>");
    }

    void configureStaticMirrors() {
        logMessage("Configuring static mirrors...");

        QFile mirrorlist("/etc/pacman.d/mirrorlist");
        if (mirrorlist.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&mirrorlist);

            if (settings["useArchMirrors"].toBool()) {
                out << "## Arch Linux repositories\n";
                out << "Server = https://mirrors.kernel.org/archlinux/$repo/os/$arch\n";
                out << "Server = https://geo.mirror.pkgbuild.com/$repo/os/$arch\n";
            }

            if (settings["useCachyosMirrors"].toBool()) {
                out << "\n## CachyOS repositories\n";
                out << "Server = https://mirror.cachyos.org/repo/$arch/$repo\n";
            }

            if (settings["useTestingMirrors"].toBool()) {
                out << "\n[testing]\n";
                out << "Include = /etc/pacman.d/mirrorlist\n";
            }

            mirrorlist.close();
        }

        logMessage("Static mirror configuration applied.");
        hintLabel->setText("<span style='color:#00ffff;'>Static mirror configuration applied.</span>");
    }

    void startInstallation() {
        // Validate all required settings
        QStringList missingFields;
        if (settings["targetDisk"].toString().isEmpty()) missingFields << "Target Disk";
        if (settings["hostname"].toString().isEmpty()) missingFields << "Hostname";
        if (settings["timezone"].toString().isEmpty()) missingFields << "Timezone";
        if (settings["keymap"].toString().isEmpty()) missingFields << "Keymap";
        if (settings["username"].toString().isEmpty()) missingFields << "Username";
        if (settings["desktopEnv"].toString().isEmpty()) missingFields << "Desktop Environment";
        if (settings["kernel"].toString().isEmpty()) missingFields << "Kernel";
        if (settings["bootloader"].toString().isEmpty()) missingFields << "Bootloader";
        if (settings["initramfs"].toString().isEmpty()) missingFields << "Initramfs";
        if (settings["compressionLevel"].toString().isEmpty()) missingFields << "Compression Level";
        if (settings["rootPassword"].toString().isEmpty()) missingFields << "Root Password";
        if (settings["userPassword"].toString().isEmpty()) missingFields << "User Password";

        if (!missingFields.isEmpty()) {
            QMessageBox::warning(this, "Error",
                                 QString("The following required fields are missing:\n%1\nPlease configure all settings before installation.")
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
            "Gaming Meta: %11\n\n"
            "Continue?")
        .arg(settings["targetDisk"].toString(),
             settings["hostname"].toString(),
             settings["timezone"].toString(),
             settings["keymap"].toString(),
             settings["username"].toString(),
             settings["desktopEnv"].toString(),
             settings["kernel"].toString(),
             settings["bootloader"].toString(),
             settings["initramfs"].toString(),
             settings["compressionLevel"].toString(),
             settings["gamingMeta"].toBool() ? "Yes" : "No");

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Confirm Installation", confirmationText,
                                      QMessageBox::Yes | QMessageBox::No);
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
        commandRunner->resetProgress();

        // Start installation process
        logMessage("Starting CachyOS BTRFS installation...");
        hintLabel->setText("<span style='color:#00ffff;'>Installation in progress. Please wait...</span>");
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

        QString disk1 = settings["targetDisk"].toString() + "1";
        QString disk2 = settings["targetDisk"].toString() + "2";
        QString compression = "zstd:" + settings["compressionLevel"].toString();

        switch (currentStep) {
            case 1: // Partitioning
                logMessage("Partitioning disk...");
                emit executeCommand("parted", {"-s", settings["targetDisk"].toString(), "mklabel", "gpt"}, true, 5);
                emit executeCommand("parted", {"-s", settings["targetDisk"].toString(), "mkpart", "primary", "1MiB", "513MiB"}, true, 5);
                emit executeCommand("parted", {"-s", settings["targetDisk"].toString(), "set", "1", "esp", "on"}, true, 5);
                emit executeCommand("parted", {"-s", settings["targetDisk"].toString(), "mkpart", "primary", "513MiB", "100%"}, true, 5);
                break;

            case 2: // Formatting
                logMessage("Formatting partitions...");
                emit executeCommand("mkfs.vfat", {"-F32", disk1}, true, 5);
                emit executeCommand("mkfs.btrfs", {"-f", disk2}, true, 5);
                break;

            case 3: // Create BTRFS subvolumes
                logMessage("Creating BTRFS subvolumes...");
                emit executeCommand("mount", {disk2, "/mnt"}, true, 2);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@"}, true, 2);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@home"}, true, 2);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@root"}, true, 2);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@srv"}, true, 2);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@tmp"}, true, 2);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@log"}, true, 2);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@cache"}, true, 2);
                emit executeCommand("umount", {"/mnt"}, true, 2);
                break;

            case 4: // Mount with compression
                logMessage("Mounting with compression...");
                emit executeCommand("mount", {"-o", QString("subvol=@,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/boot/efi"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/etc"}, true);
                emit executeCommand("touch", {"/mnt/etc/fstab"}, true);
                emit executeCommand("mount", {disk1, "/mnt/boot/efi"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/home"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@home,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/home"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/root"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@root,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/root"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/srv"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@srv,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/srv"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/tmp"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@tmp,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/tmp"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/var/log"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@log,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/var/log"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/var/cache"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@cache,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/var/cache"}, true);
                break;

            case 6: // Generate fstab
                logMessage("Generating fstab...");
                emit executeCommand("cp", {"btrfsfstabcompressed.sh", "/mnt/btrfsfstabcompressed.sh"}, true, 2);
                emit executeCommand("chmod", {"+x", "/mnt/btrfsfstabcompressed.sh"}, true, 1);
                emit executeCommand("arch-chroot", {"/mnt", "./btrfsfstabcompressed.sh"}, true, 5);
                break;

            case 7: // Enable selected repositories
                logMessage("Enabling selected repositories...");
                if (settings["useCachyosMirrors"].toBool()) {
                    emit executeCommand("arch-chroot", {"/mnt", "pacman-key", "--recv-keys", "F3B607488DB35A47", "--keyserver", "keyserver.ubuntu.com"}, true, 2);
                    emit executeCommand("arch-chroot", {"/mnt", "pacman-key", "--lsign-key", "F3B607488DB35A47"}, true, 2);
                }
                break;

            case 8: // Mount required filesystems for chroot
                logMessage("Preparing chroot environment...");
                emit executeCommand("mount", {"-t", "proc", "none", "/mnt/proc"}, true, 1);
                emit executeCommand("mount", {"--rbind", "/dev", "/mnt/dev"}, true, 1);
                emit executeCommand("mount", {"--rbind", "/sys", "/mnt/sys"}, true, 1);
                break;

            case 9: // Create chroot setup script
                logMessage("Preparing chroot setup script...");
                createChrootScript();
                emit executeCommand("chmod", {"+x", "/mnt/setup-chroot.sh"}, true, 1);
                break;

            case 10: // Run chroot setup
                logMessage("Running chroot setup...");
                emit executeCommand("arch-chroot", {"/mnt", "./setup-chroot.sh"}, true, 20);
                break;

            case 11: // Clean up
                logMessage("Cleaning up...");
                emit executeCommand("umount", {"-R", "/mnt"}, true, 5);
                break;

            case 12: // Installation complete
                logMessage("Installation complete!");
                progressBar->setValue(100);
                hintLabel->setText("<span style='color:#00ffff;'>Installation complete! Select post-install options.</span>");
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
            hintLabel->setText("<span style='color:#ff0000;'>Installation failed! Check the log for details.</span>");
            return;
        }

        // Proceed to next step
        QTimer::singleShot(500, this, &CachyOSInstaller::nextInstallationStep);
    }

    void createChrootScript() {
        QFile scriptFile("/mnt/setup-chroot.sh");
        if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&scriptFile);
            out << "#!/bin/bash\n";
            out << "# Basic system configuration\n";
            out << "echo \"" << settings["hostname"].toString() << "\" > /etc/hostname\n";
            out << "ln -sf /usr/share/zoneinfo/" << settings["timezone"].toString() << " /etc/localtime\n";
            out << "hwclock --systohc\n";
            out << "echo \"en_US.UTF-8 UTF-8\" >> /etc/locale.gen\n";
            out << "locale-gen\n";
            out << "echo \"LANG=en_US.UTF-8\" > /etc/locale.conf\n";
            out << "echo \"KEYMAP=" << settings["keymap"].toString() << "\" > /etc/vconsole.conf\n";
            out << "# Users and passwords\n";
            out << "echo \"root:" << settings["rootPassword"].toString() << "\" | chpasswd\n";
            out << "useradd -m -G wheel,audio,video,storage,optical -s /bin/bash " << settings["username"].toString() << "\n";
            out << "echo \"" << settings["username"].toString() << ":" << settings["userPassword"].toString() << "\" | chpasswd\n";
            out << "# Configure sudo\n";
            out << "echo \"%wheel ALL=(ALL) ALL\" > /etc/sudoers.d/wheel\n";

            // Handle bootloader installation
            out << "# Install bootloader\n";
            if (settings["bootloader"].toString() == "GRUB") {
                out << "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=CachyOS\n";
                out << "grub-mkconfig -o /boot/grub/grub.cfg\n";
            } else if (settings["bootloader"].toString() == "systemd-boot") {
                out << "bootctl --path=/boot/efi install\n";
                out << "mkdir -p /boot/efi/loader/entries\n";
                out << "cat > /boot/efi/loader/loader.conf << 'LOADER'\n";
                out << "default arch\n";
                out << "timeout 3\n";
                out << "editor  yes\n";
                out << "LOADER\n";
                out << "cat > /boot/efi/loader/entries/arch.conf << 'ENTRY'\n";
                out << "title   CachyOS Linux\n";
                out << "linux   /vmlinuz-" << settings["kernel"].toString() << "\n";
                out << "initrd  /initramfs-" << settings["kernel"].toString() << ".img\n";
                out << "options root=UUID=" << getDiskUuid(settings["targetDisk"].toString() + "2") << " rootflags=subvol=@ rw\n";
                out << "ENTRY\n";
            } else if (settings["bootloader"].toString() == "rEFInd") {
                out << "refind-install\n";
                out << "mkdir -p /boot/efi/EFI/refind/refind.conf\n";
                out << "cat > /boot/efi/EFI/refind/refind.conf << 'REFIND'\n";
                out << "menuentry \"CachyOS Linux\" {\n";
                out << "    icon     /EFI/refind/icons/os_arch.png\n";
                out << "    loader   /vmlinuz-" << settings["kernel"].toString() << "\n";
                out << "    initrd   /initramfs-" << settings["kernel"].toString() << ".img\n";
                out << "    options  \"root=UUID=" << getDiskUuid(settings["targetDisk"].toString() + "2") << " rootflags=subvol=@ rw\"\n";
                out << "}\n";
                out << "REFIND\n";
            }

            // Handle initramfs
            out << "\n# Generate initramfs\n";
            if (settings["initramfs"].toString() == "mkinitcpio") {
                out << "mkinitcpio -P\n";
            } else if (settings["initramfs"].toString() == "dracut") {
                out << "dracut --regenerate-all --force\n";
            } else if (settings["initramfs"].toString() == "booster") {
                out << "booster generate\n";
            } else if (settings["initramfs"].toString() == "mkinitcpio-pico") {
                out << "mkinitcpio -P\n";
            }

            // Network manager (only enable if no desktop selected)
            if (settings["desktopEnv"].toString() == "None") {
                out << "\n# Enable NetworkManager\n";
                out << "systemctl enable NetworkManager\n";
            }

            // Install desktop environment and related packages
            out << "\n# Install desktop environment\n";
            if (settings["desktopEnv"].toString() == "KDE Plasma") {
                out << "pacman -S --noconfirm plasma-meta kde-applications-meta sddm cachyos-kde-settings --disable-download-timeout\n";
                out << "systemctl enable sddm\n";
                out << "pacman -S --noconfirm firefox dolphin konsole --disable-download-timeout\n";
            } else if (settings["desktopEnv"].toString() == "GNOME") {
                out << "pacman -S --noconfirm gnome gnome-extra gdm --disable-download-timeout\n";
                out << "systemctl enable gdm\n";
                out << "pacman -S --noconfirm firefox gnome-terminal --disable-download-timeout\n";
            } else if (settings["desktopEnv"].toString() == "XFCE") {
                out << "pacman -S --noconfirm xfce4 xfce4-goodies lightdm lightdm-gtk-greeter --disable-download-timeout\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox mousepad xfce4-terminal --disable-download-timeout\n";
            } else if (settings["desktopEnv"].toString() == "MATE") {
                out << "pacman -S --noconfirm mate mate-extra mate-media lightdm lightdm-gtk-greeter --disable-download-timeout\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox pluma mate-terminal --disable-download-timeout\n";
            } else if (settings["desktopEnv"].toString() == "LXQt") {
                out << "pacman -S --noconfirm lxqt breeze-icons sddm --disable-download-timeout\n";
                out << "systemctl enable sddm\n";
                out << "pacman -S --noconfirm firefox qterminal --disable-download-timeout\n";
            } else if (settings["desktopEnv"].toString() == "Cinnamon") {
                out << "pacman -S --noconfirm cinnamon cinnamon-translations lightdm lightdm-gtk-greeter --disable-download-timeout\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox xed gnome-terminal --disable-download-timeout\n";
            } else if (settings["desktopEnv"].toString() == "Budgie") {
                out << "pacman -S --noconfirm budgie-desktop budgie-extras gnome-control-center gnome-terminal lightdm lightdm-gtk-greeter --disable-download-timeout\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox gnome-text-editor gnome-terminal --disable-download-timeout\n";
            } else if (settings["desktopEnv"].toString() == "Deepin") {
                out << "pacman -S --noconfirm deepin deepin-extra lightdm --disable-download-timeout\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox deepin-terminal --disable-download-timeout\n";
            } else if (settings["desktopEnv"].toString() == "i3") {
                out << "pacman -S --noconfirm i3-wm i3status i3lock dmenu lightdm lightdm-gtk-greeter --disable-download-timeout\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox alacritty --disable-download-timeout\n";
            } else if (settings["desktopEnv"].toString() == "Sway") {
                out << "pacman -S --noconfirm sway swaylock swayidle waybar wofi lightdm lightdm-gtk-greeter --disable-download-timeout\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox foot --disable-download-timeout\n";
            } else if (settings["desktopEnv"].toString() == "Hyprland") {
                out << "pacman -S --noconfirm hyprland waybar rofi wofi kitty swaybg swaylock-effects wl-clipboard lightdm lightdm-gtk-greeter --disable-download-timeout\n";
                out << "systemctl enable lightdm\n";
                out << "pacman -S --noconfirm firefox kitty --disable-download-timeout\n";

                // Create Hyprland config directory
                out << "# Create Hyprland config directory\n";
                out << "mkdir -p /home/" << settings["username"].toString() << "/.config/hypr\n";
                out << "cat > /home/" << settings["username"].toString() << "/.config/hypr/hyprland.conf << 'HYPRCONFIG'\n";
                out << "# This is a basic Hyprland config\n";
                out << "exec-once = waybar &\n";
                out << "exec-once = swaybg -i ~/wallpaper.jpg &\n";
                out << "monitor=,preferred,auto,1\n";
                out << "input {\n";
                out << "    kb_layout = us\n";
                out << "    follow_mouse = 1\n";
                out << "    touchpad {\n";
                out << "        natural_scroll = yes\n";
                out << "    }\n";
                out << "}\n";
                out << "general {\n";
                out << "    gaps_in = 5\n";
                out << "    gaps_out = 10\n";
                out << "    border_size = 2\n";
                out << "    col.active_border = rgba(33ccffee) rgba(00ff99ee) 45deg\n";
                out << "    col.inactive_border = rgba(595959aa)\n";
                out << "}\n";
                out << "decoration {\n";
                out << "    rounding = 5\n";
                out << "    blur = yes\n";
                out << "    blur_size = 3\n";
                out << "    blur_passes = 1\n";
                out << "    blur_new_optimizations = on\n";
                out << "}\n";
                out << "animations {\n";
                out << "    enabled = yes\n";
                out << "    bezier = myBezier, 0.05, 0.9, 0.1, 1.05\n";
                out << "    animation = windows, 1, 7, myBezier\n";
                out << "    animation = windowsOut, 1, 7, default, popin 80%\n";
                out << "    animation = border, 1, 10, default\n";
                out << "    animation = fade, 1, 7, default\n";
                out << "    animation = workspaces, 1, 6, default\n";
                out << "}\n";
                out << "dwindle {\n";
                out << "    pseudotile = yes\n";
                out << "    preserve_split = yes\n";
                out << "}\n";
                out << "master {\n";
                out << "    new_is_master = true\n";
                out << "}\n";
                out << "bind = SUPER, Return, exec, kitty\n";
                out << "bind = SUPER, Q, killactive,\n";
                out << "bind = SUPER, M, exit,\n";
                out << "bind = SUPER, V, togglefloating,\n";
                out << "bind = SUPER, F, fullscreen,\n";
                out << "bind = SUPER, D, exec, rofi -show drun\n";
                out << "bind = SUPER, P, pseudo,\n";
                out << "bind = SUPER, J, togglesplit,\n";
                out << "HYPRCONFIG\n";

                // Set ownership of config files
                out << "# Set ownership of config files\n";
                out << "chown -R " << settings["username"].toString() << ":" << settings["username"].toString()
                << " /home/" << settings["username"].toString() << "/.config\n";
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

            scriptFile.close();
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
            QProcess::execute("sudo", QStringList() << "-S" << "reboot");
            dialog.accept();
        });

        connect(chrootButton, &QPushButton::clicked, [this, &dialog]() {
            logMessage("Entering chroot...");
            QString disk1 = settings["targetDisk"].toString() + "1";
            QString disk2 = settings["targetDisk"].toString() + "2";
            emit executeCommand("mount", {disk1, "/mnt/boot/efi"}, true, 0);
            emit executeCommand("mount", {"-o", "subvol=@", disk2, "/mnt"}, true, 0);
            emit executeCommand("mount", {"-t", "proc", "none", "/mnt/proc"}, true, 0);
            emit executeCommand("mount", {"--rbind", "/dev", "/mnt/dev"}, true, 0);
            emit executeCommand("mount", {"--rbind", "/sys", "/mnt/sys"}, true, 0);
            emit executeCommand("mount", {"--rbind", "/dev/pts", "/mnt/dev/pts"}, true, 0);
            emit executeCommand("arch-chroot", {"/mnt"}, true, 0);
            emit executeCommand("umount", {"-l", "/mnt"}, true, 0);
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
        process.start("sudo", QStringList() << "-S" << "blkid" << "-s" << "UUID" << "-o" << "value" << disk);
        process.write((commandRunner->password() + "\n").toUtf8());
        process.closeWriteChannel();
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
        settings["useArchMirrors"] = true;
        settings["useCachyosMirrors"] = true;
        settings["useFastestMirrors"] = false;
        settings["useTestingMirrors"] = false;
    }

    QProgressBar *progressBar;
    QTextEdit *logArea;
    QLabel *hintLabel;
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
