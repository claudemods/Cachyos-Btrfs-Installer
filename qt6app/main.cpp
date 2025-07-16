#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QProcess>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QSettings>
#include <QTimer>
#include <QScrollBar>
#include <QProgressBar>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QRegularExpression>
#include <QPalette>
#include <QComboBox>
#include <QFormLayout>
#include <QFile>
#include <QSpinBox>
#include <QDialog>

// ANSI color codes
const QString COLOR_CYAN = "\033[38;2;0;255;255m";
const QString COLOR_RED = "\033[38;2;255;0;0m";
const QString COLOR_RESET = "\033[0m";

class PasswordDialog : public QDialog {
    Q_OBJECT
public:
    PasswordDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Enter Sudo Password");
        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *label = new QLabel("Enter your sudo password to continue installation:");
        label->setStyleSheet("color: cyan;");
        layout->addWidget(label);

        passwordEdit = new QLineEdit(this);
        passwordEdit->setEchoMode(QLineEdit::Password);
        passwordEdit->setStyleSheet("color: cyan;");
        layout->addWidget(passwordEdit);

        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->setStyleSheet("color: cyan;");
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
    }

    QString password() const { return passwordEdit->text(); }

private:
    QLineEdit *passwordEdit;
};

class PasswordConfigDialog : public QDialog {
    Q_OBJECT
public:
    PasswordConfigDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("User and Root Passwords");
        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *userLabel = new QLabel("User Password:");
        userLabel->setStyleSheet("color: cyan;");
        layout->addWidget(userLabel);

        userPasswordEdit = new QLineEdit(this);
        userPasswordEdit->setEchoMode(QLineEdit::Password);
        userPasswordEdit->setStyleSheet("color: cyan;");
        layout->addWidget(userPasswordEdit);

        QLabel *rootLabel = new QLabel("Root Password:");
        rootLabel->setStyleSheet("color: cyan;");
        layout->addWidget(rootLabel);

        rootPasswordEdit = new QLineEdit(this);
        rootPasswordEdit->setEchoMode(QLineEdit::Password);
        rootPasswordEdit->setStyleSheet("color: cyan;");
        layout->addWidget(rootPasswordEdit);

        samePasswordCheck = new QCheckBox("Use same password for root", this);
        samePasswordCheck->setStyleSheet("color: cyan;");
        connect(samePasswordCheck, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
            if (state == Qt::Checked) {
                rootPasswordEdit->setText(userPasswordEdit->text());
                rootPasswordEdit->setEnabled(false);
            } else {
                rootPasswordEdit->setEnabled(true);
            }
        });
        layout->addWidget(samePasswordCheck);

        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->setStyleSheet("color: cyan;");
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
    }

    QString getUserPassword() const { return userPasswordEdit->text(); }
    QString getRootPassword() const { return rootPasswordEdit->text(); }

private:
    QLineEdit *userPasswordEdit;
    QLineEdit *rootPasswordEdit;
    QCheckBox *samePasswordCheck;
};

class LogViewer : public QDialog {
    Q_OBJECT
public:
    LogViewer(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Installation Log");
        resize(800, 600);

        QVBoxLayout *layout = new QVBoxLayout(this);

        logText = new QTextEdit(this);
        logText->setReadOnly(true);
        logText->setStyleSheet("background-color: black; color: cyan;");
        layout->addWidget(logText);

        QPushButton *clearButton = new QPushButton("Clear Log", this);
        clearButton->setStyleSheet("color: cyan;");
        connect(clearButton, &QPushButton::clicked, this, &LogViewer::clearLog);
        layout->addWidget(clearButton);

        QPushButton *saveButton = new QPushButton("Save Log", this);
        saveButton->setStyleSheet("color: cyan;");
        connect(saveButton, &QPushButton::clicked, this, &LogViewer::saveLog);
        layout->addWidget(saveButton);
    }

    void appendLog(const QString &text) {
        logText->append(text);
        logText->verticalScrollBar()->setValue(logText->verticalScrollBar()->maximum());
    }

    void clearLog() {
        logText->clear();
    }

    void saveLog() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Log File", "", "Text Files (*.txt)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << logText->toPlainText();
                file.close();
                QMessageBox::information(this, "Success", "Log saved successfully");
            } else {
                QMessageBox::warning(this, "Error", "Could not save log file");
            }
        }
    }

private:
    QTextEdit *logText;
};

class CachyOSInstaller : public QMainWindow {
    Q_OBJECT

public:
    CachyOSInstaller(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupUI();
        loadSettings();
    }

private slots:
    void onConfigMenuClicked() {
        configureInstallation();
    }

    void onSetupMirrorsClicked() {
        configureFastestMirrors();
    }

    void onInstallClicked() {
        if (targetDisk.isEmpty()) {
            appendToConsole(COLOR_RED + "Please configure installation first!" + COLOR_RESET);
            return;
        }

        PasswordDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            sudoPassword = dialog.password();
            performInstallation();
        }
    }

    void onLogClicked() {
        logViewer->show();
    }

    void onExitClicked() {
        saveSettings();
        QApplication::quit();
    }

private:
    void setupUI() {
        // Main widgets
        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        // ASCII Art
        asciiArt = new QLabel(this);
        asciiArt->setText(
            "<pre><span style='color:#ff0000;'>"
            "░█████╗░██╗░░░░░░█████╗░██╗░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗<br>"
            "██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝<br>"
            "██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░<br>"
            "██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗<br>"
            "╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝<br>"
            "░╚════╝░╚══════╝╚═╝░░╚═╝░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░</span><br>"
            "<span style='color:#00ffff;'>CachyOS Btrfs Installer v1.0 15-07-2025</span></pre>"
        );
        asciiArt->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(asciiArt);

        // Progress bar
        progressBar = new QProgressBar(this);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        progressBar->setTextVisible(true);
        progressBar->setStyleSheet("QProgressBar { color: cyan; }");
        mainLayout->addWidget(progressBar);

        // Buttons
        QHBoxLayout *buttonLayout = new QHBoxLayout();

        QPushButton *configBtn = new QPushButton("1. Config Menu", this);
        QPushButton *mirrorsBtn = new QPushButton("2. Setup Mirrors", this);
        QPushButton *installBtn = new QPushButton("3. Install", this);
        QPushButton *logBtn = new QPushButton("4. Log", this);
        QPushButton *exitBtn = new QPushButton("5. Exit", this);

        // Set button text color to cyan
        QString buttonStyle = "QPushButton { color: cyan; }";
        configBtn->setStyleSheet(buttonStyle);
        mirrorsBtn->setStyleSheet(buttonStyle);
        installBtn->setStyleSheet(buttonStyle);
        logBtn->setStyleSheet(buttonStyle);
        exitBtn->setStyleSheet(buttonStyle);

        connect(configBtn, &QPushButton::clicked, this, &CachyOSInstaller::onConfigMenuClicked);
        connect(mirrorsBtn, &QPushButton::clicked, this, &CachyOSInstaller::onSetupMirrorsClicked);
        connect(installBtn, &QPushButton::clicked, this, &CachyOSInstaller::onInstallClicked);
        connect(logBtn, &QPushButton::clicked, this, &CachyOSInstaller::onLogClicked);
        connect(exitBtn, &QPushButton::clicked, this, &CachyOSInstaller::onExitClicked);

        buttonLayout->addWidget(configBtn);
        buttonLayout->addWidget(mirrorsBtn);
        buttonLayout->addWidget(installBtn);
        buttonLayout->addWidget(logBtn);
        buttonLayout->addWidget(exitBtn);

        mainLayout->addLayout(buttonLayout);

        // Output console
        console = new QTextEdit(this);
        console->setReadOnly(true);
        console->setStyleSheet("background-color: black; color: cyan;");
        mainLayout->addWidget(console);

        // Log viewer
        logViewer = new LogViewer(this);

        setCentralWidget(centralWidget);
        resize(800, 600);
        setWindowTitle("CachyOS Btrfs Installer");
    }

    void configureInstallation() {
        // Create a dialog for configuration
        QDialog dialog(this);
        dialog.setWindowTitle("Configuration Menu");
        QVBoxLayout *layout = new QVBoxLayout(&dialog);

        // Set cyan color for all text in the dialog
        dialog.setStyleSheet("color: cyan;");

        // Add configuration widgets
        QLineEdit *targetDiskEdit = new QLineEdit(targetDisk, &dialog);
        QLineEdit *hostnameEdit = new QLineEdit(hostname, &dialog);
        QLineEdit *timezoneEdit = new QLineEdit(timezone, &dialog);
        QLineEdit *keymapEdit = new QLineEdit(keymap, &dialog);
        QLineEdit *userNameEdit = new QLineEdit(userName, &dialog);

        // Password configuration
        PasswordConfigDialog passwordDialog(this);
        if (passwordDialog.exec() == QDialog::Accepted) {
            userPassword = passwordDialog.getUserPassword();
            rootPassword = passwordDialog.getRootPassword();
        } else {
            return;
        }

        // Kernel selection
        QStringList kernels = {"Bore", "Bore-Extra", "CachyOS", "CachyOS-Extra", "LTS", "Zen"};
        QComboBox *kernelCombo = new QComboBox(&dialog);
        kernelCombo->addItems(kernels);
        kernelCombo->setCurrentText(kernelType);

        // Initramfs selection
        QStringList initramfsOptions = {"mkinitcpio", "dracut", "booster", "mkinitcpio-pico"};
        QComboBox *initramfsCombo = new QComboBox(&dialog);
        initramfsCombo->addItems(initramfsOptions);
        initramfsCombo->setCurrentText(initramfs);

        // Bootloader selection
        QStringList bootloaders = {"GRUB", "systemd-boot", "rEFInd"};
        QComboBox *bootloaderCombo = new QComboBox(&dialog);
        bootloaderCombo->addItems(bootloaders);
        bootloaderCombo->setCurrentText(bootloader);

        // Repository selection
        QGroupBox *repoGroup = new QGroupBox("Additional Repositories", &dialog);
        QVBoxLayout *repoLayout = new QVBoxLayout(repoGroup);
        QStringList repos = {"multilib", "testing", "community-testing", "cachyos", "cachyos-v3", "cachyos-testing"};
        QList<QCheckBox*> repoCheckboxes;
        for (const QString &repo : repos) {
            QCheckBox *check = new QCheckBox(repo, repoGroup);
            check->setChecked(repositories.contains(repo));
            repoLayout->addWidget(check);
            repoCheckboxes.append(check);
        }

        // Desktop environment
        QStringList desktops = {
            "KDE Plasma", "GNOME", "XFCE", "MATE", "LXQt", "Cinnamon",
            "Budgie", "Deepin", "i3", "Sway", "Hyprland", "None"
        };
        QComboBox *desktopCombo = new QComboBox(&dialog);
        desktopCombo->addItems(desktops);
        desktopCombo->setCurrentText(desktopEnv);

        // Compression level
        QSpinBox *compressionSpin = new QSpinBox(&dialog);
        compressionSpin->setRange(0, 22);
        compressionSpin->setValue(compressionLevel);

        // Form layout
        QFormLayout *formLayout = new QFormLayout();
        formLayout->addRow("Target Disk:", targetDiskEdit);
        formLayout->addRow("Hostname:", hostnameEdit);
        formLayout->addRow("Timezone:", timezoneEdit);
        formLayout->addRow("Keymap:", keymapEdit);
        formLayout->addRow("Username:", userNameEdit);
        formLayout->addRow("Kernel:", kernelCombo);
        formLayout->addRow("Initramfs:", initramfsCombo);
        formLayout->addRow("Bootloader:", bootloaderCombo);
        formLayout->addRow("Desktop Environment:", desktopCombo);
        formLayout->addRow("BTRFS Compression Level (0-22):", compressionSpin);

        layout->addLayout(formLayout);
        layout->addWidget(repoGroup);

        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        layout->addWidget(buttons);

        if (dialog.exec() == QDialog::Accepted) {
            targetDisk = targetDiskEdit->text();
            hostname = hostnameEdit->text();
            timezone = timezoneEdit->text();
            keymap = keymapEdit->text();
            userName = userNameEdit->text();
            kernelType = kernelCombo->currentText();
            initramfs = initramfsCombo->currentText();
            bootloader = bootloaderCombo->currentText();
            desktopEnv = desktopCombo->currentText();
            compressionLevel = compressionSpin->value();

            repositories.clear();
            for (QCheckBox *check : repoCheckboxes) {
                if (check->isChecked()) {
                    repositories.append(check->text());
                }
            }

            appendToConsole("Installation configuration saved.");
        }
    }

    void configureFastestMirrors() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Fastest Mirrors", "Would you like to find and use the fastest mirrors?",
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            appendToConsole(COLOR_CYAN + "Finding fastest mirrors..." + COLOR_RESET);

            // Execute reflector command
            executeCommand("pacman -Sy --noconfirm reflector");
            executeCommand("reflector --latest 20 --protocol https --sort rate --save /etc/pacman.d/mirrorlist");

            appendToConsole(COLOR_CYAN + "Mirrorlist updated with fastest mirrors" + COLOR_RESET);
        } else {
            appendToConsole(COLOR_CYAN + "Using current mirrors" + COLOR_RESET);
        }
    }

    void performInstallation() {
        // Check if running as root
        if (QProcess::execute("id", {"-u"}) != 0) {
            appendToConsole(COLOR_RED + "This script must be run as root or with sudo" + COLOR_RESET);
            return;
        }

        // Check UEFI boot mode
        if (!QFile::exists("/sys/firmware/efi")) {
            appendToConsole(COLOR_RED + "ERROR: This script requires UEFI boot mode" + COLOR_RESET);
            return;
        }

        // Show confirmation dialog
        QString confirmation = QString(
            "About to install to %1 with these settings:\n"
            "Hostname: %2\n"
            "Timezone: %3\n"
            "Keymap: %4\n"
            "Username: %5\n"
            "Desktop: %6\n"
            "Kernel: %7\n"
            "Initramfs: %8\n"
            "Bootloader: %9\n"
            "Repositories: %10\n"
            "Compression Level: %11\n\n"
            "Continue?"
        ).arg(targetDisk, hostname, timezone, keymap, userName, desktopEnv, kernelType,
              initramfs, bootloader, repositories.join(","), QString::number(compressionLevel));

        QMessageBox::StandardButton confirm = QMessageBox::question(
            this, "Confirm Installation", confirmation,
            QMessageBox::Yes | QMessageBox::No
        );

        if (confirm != QMessageBox::Yes) {
            appendToConsole(COLOR_CYAN + "Installation cancelled." + COLOR_RESET);
            return;
        }

        // Start installation process
        appendToConsole(COLOR_CYAN + "Starting installation process..." + COLOR_RESET);
        progressBar->setValue(5);

        // Partitioning
        executeCommand(QString("parted -s %1 mklabel gpt").arg(targetDisk));
        executeCommand(QString("parted -s %1 mkpart primary 1MiB 513MiB").arg(targetDisk));
        executeCommand(QString("parted -s %1 set 1 esp on").arg(targetDisk));
        executeCommand(QString("parted -s %1 mkpart primary 513MiB 100%").arg(targetDisk));
        progressBar->setValue(10);

        // Formatting
        executeCommand(QString("mkfs.vfat -F32 %11").arg(targetDisk));
        executeCommand(QString("mkfs.btrfs -f %12").arg(targetDisk));
        progressBar->setValue(15);

        // Mounting and subvolumes
        executeCommand(QString("mount %12 /mnt").arg(targetDisk));
        executeCommand("btrfs subvolume create /mnt/@");
        executeCommand("btrfs subvolume create /mnt/@home");
        executeCommand("btrfs subvolume create /mnt/@root");
        executeCommand("btrfs subvolume create /mnt/@srv");
        executeCommand("btrfs subvolume create /mnt/@cache");
        executeCommand("btrfs subvolume create /mnt/@tmp");
        executeCommand("btrfs subvolume create /mnt/@log");
        executeCommand("umount /mnt");
        progressBar->setValue(20);

        // Remount with compression
        executeCommand(QString("mount -o subvol=@,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt").arg(compressionLevel).arg(targetDisk));
        executeCommand("mkdir -p /mnt/boot/efi");
        executeCommand(QString("mount %11 /mnt/boot/efi").arg(targetDisk));
        executeCommand("mkdir -p /mnt/home");
        executeCommand("mkdir -p /mnt/root");
        executeCommand("mkdir -p /mnt/srv");
        executeCommand("mkdir -p /mnt/tmp");
        executeCommand("mkdir -p /mnt/var/cache");
        executeCommand("mkdir -p /mnt/var/log");
        executeCommand(QString("mount -o subvol=@home,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt/home").arg(compressionLevel).arg(targetDisk));
        executeCommand(QString("mount -o subvol=@root,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt/root").arg(compressionLevel).arg(targetDisk));
        executeCommand(QString("mount -o subvol=@srv,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt/srv").arg(compressionLevel).arg(targetDisk));
        executeCommand(QString("mount -o subvol=@tmp,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt/tmp").arg(compressionLevel).arg(targetDisk));
        executeCommand(QString("mount -o subvol=@cache,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt/var/cache").arg(compressionLevel).arg(targetDisk));
        executeCommand(QString("mount -o subvol=@log,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt/var/log").arg(compressionLevel).arg(targetDisk));
        progressBar->setValue(30);

        // Determine kernel package
        QString kernelPkg;
        if (kernelType == "Bore") kernelPkg = "linux-cachyos-bore";
        else if (kernelType == "Bore-Extra") kernelPkg = "linux-cachyos-bore-extra";
        else if (kernelType == "CachyOS") kernelPkg = "linux-cachyos";
        else if (kernelType == "CachyOS-Extra") kernelPkg = "linux-cachyos-extra";
        else if (kernelType == "LTS") kernelPkg = "linux-lts";
        else if (kernelType == "Zen") kernelPkg = "linux-zen";

        // Base packages using pacstrap
        QString basePkgs = QString("base %1 linux-firmware btrfs-progs nano").arg(kernelPkg);

        // Add bootloader packages
        if (bootloader == "GRUB") {
            basePkgs += " grub efibootmgr dosfstools cachyos-grub-theme";
        } else if (bootloader == "systemd-boot") {
            basePkgs += " efibootmgr";
        } else if (bootloader == "rEFInd") {
            basePkgs += " refind";
        }

        // Add initramfs packages
        if (initramfs == "mkinitcpio") {
            basePkgs += " mkinitcpio";
        } else if (initramfs == "dracut") {
            basePkgs += " dracut";
        } else if (initramfs == "booster") {
            basePkgs += " booster";
        } else if (initramfs == "mkinitcpio-pico") {
            basePkgs += " mkinitcpio-pico";
        }

        // Add network manager if no desktop selected
        if (desktopEnv == "None") {
            basePkgs += " networkmanager";
        }

        executeCommand(QString("pacstrap -i /mnt %1 --noconfirm --disable-download-timeout").arg(basePkgs));
        progressBar->setValue(40);

        // Add selected repositories
        for (const QString &repo : repositories) {
            if (repo == "multilib") {
                appendToConsole(COLOR_CYAN + "Enabling multilib repository..." + COLOR_RESET);
                executeCommand("sed -i '/\\[multilib\\]/,/Include/s/^#//' /mnt/etc/pacman.conf");
            } else if (repo == "testing") {
                appendToConsole(COLOR_CYAN + "Enabling testing repository..." + COLOR_RESET);
                executeCommand("sed -i '/\\[testing\\]/,/Include/s/^#//' /mnt/etc/pacman.conf");
            } else if (repo == "community-testing") {
                appendToConsole(COLOR_CYAN + "Enabling community-testing repository..." + COLOR_RESET);
                executeCommand("sed -i '/\\[community-testing\\]/,/Include/s/^#//' /mnt/etc/pacman.conf");
            } else if (repo == "cachyos") {
                appendToConsole(COLOR_CYAN + "Enabling CachyOS repository..." + COLOR_RESET);
                executeCommand("if ! grep -q \"\\[cachyos\\]\" /mnt/etc/pacman.conf; then "
                "echo -e \"\\n[cachyos]\\nServer = https://mirror.cachyos.org/repo/\\$arch/\\$repo\" >> /mnt/etc/pacman.conf; "
                "else sed -i '/\\[cachyos\\]/,/Include/s/^#//' /mnt/etc/pacman.conf; fi");
            } else if (repo == "cachyos-v3") {
                appendToConsole(COLOR_CYAN + "Enabling CachyOS V3 repository..." + COLOR_RESET);
                executeCommand("if ! grep -q \"\\[cachyos-v3\\]\" /mnt/etc/pacman.conf; then "
                "echo -e \"\\n[cachyos-v3]\\nServer = https://mirror.cachyos.org/repo-v3/\\$arch/\\$repo\" >> /mnt/etc/pacman.conf; "
                "else sed -i '/\\[cachyos-v3\\]/,/Include/s/^#//' /mnt/etc/pacman.conf; fi");
            } else if (repo == "cachyos-testing") {
                appendToConsole(COLOR_CYAN + "Enabling CachyOS Testing repository..." + COLOR_RESET);
                executeCommand("if ! grep -q \"\\[cachyos-testing\\]\" /mnt/etc/pacman.conf; then "
                "echo -e \"\\n[cachyos-testing]\\nServer = https://mirror.cachyos.org/repo-testing/\\$arch/\\$repo\" >> /mnt/etc/pacman.conf; "
                "else sed -i '/\\[cachyos-testing\\]/,/Include/s/^#//' /mnt/etc/pacman.conf; fi");
            }
        }

        // Import CachyOS key if needed
        if (repositories.contains("cachyos") || repositories.contains("cachyos-v3") || repositories.contains("cachyos-testing")) {
            appendToConsole(COLOR_CYAN + "Importing CachyOS key..." + COLOR_RESET);
            executeCommand("arch-chroot /mnt bash -c \"pacman-key --recv-keys F3B607488DB35A47 --keyserver keyserver.ubuntu.com\"");
            executeCommand("arch-chroot /mnt bash -c \"pacman-key --lsign-key F3B607488DB35A47\"");
        }
        progressBar->setValue(50);

        // Generate fstab
        appendToConsole(COLOR_CYAN + "Generating fstab with BTRFS subvolumes..." + COLOR_RESET);
        QProcess process;
        process.start("blkid", {"-s", "UUID", "-o", "value", QString("%12").arg(targetDisk)});
        process.waitForFinished();
        QString rootUuid = process.readAllStandardOutput().trimmed();

        QString fstabContent = QString(
            "\n# Btrfs subvolumes (auto-added)\n"
            "UUID=%1 /              btrfs   rw,noatime,compress=zstd:%2,discard=async,space_cache=v2,subvol=/@ 0 0\n"
            "UUID=%1 /root          btrfs   rw,noatime,compress=zstd:%2,discard=async,space_cache=v2,subvol=/@root 0 0\n"
            "UUID=%1 /home          btrfs   rw,noatime,compress=zstd:%2,discard=async,space_cache=v2,subvol=/@home 0 0\n"
            "UUID=%1 /srv           btrfs   rw,noatime,compress=zstd:%2,discard=async,space_cache=v2,subvol=/@srv 0 0\n"
            "UUID=%1 /var/cache     btrfs   rw,noatime,compress=zstd:%2,discard=async,space_cache=v2,subvol=/@cache 0 0\n"
            "UUID=%1 /var/tmp       btrfs   rw,noatime,compress=zstd:%2,discard=async,space_cache=v2,subvol=/@tmp 0 0\n"
            "UUID=%1 /var/log       btrfs   rw,noatime,compress=zstd:%2,discard=async,space_cache=v2,subvol=/@log 0 0\n"
            "UUID=%1 /var/lib/portables btrfs rw,noatime,compress=zstd:%2,discard=async,space_cache=v2,subvol=/@/var/lib/portables 0 0\n"
            "UUID=%1 /var/lib/machines btrfs rw,noatime,compress=zstd:%2,discard=async,space_cache=v2,subvol=/@/var/lib/machines 0 0\n"
        ).arg(rootUuid, QString::number(compressionLevel));

        executeCommand(QString("echo \"%1\" >> /mnt/etc/fstab").arg(fstabContent));
        progressBar->setValue(60);

        // Create chroot setup script
        QString chrootScript = QString(
            "#!/bin/bash\n\n"
            "# Basic system configuration\n"
            "echo \"%1\" > /etc/hostname\n"
            "ln -sf /usr/share/zoneinfo/%2 /etc/localtime\n"
            "hwclock --systohc\n"
            "echo \"en_US.UTF-8 UTF-8\" >> /etc/locale.gen\n"
            "locale-gen\n"
            "echo \"LANG=en_US.UTF-8\" > /etc/locale.conf\n"
            "echo \"KEYMAP=%3\" > /etc/vconsole.conf\n\n"
            "# Users and passwords\n"
            "echo \"root:%4\" | chpasswd\n"
            "useradd -m -G wheel,audio,video,storage,optical -s /bin/bash \"%5\"\n"
            "echo \"%5:%6\" | chpasswd\n\n"
            "# Configure sudo\n"
            "echo \"%%wheel ALL=(ALL) ALL\" > /etc/sudoers.d/wheel\n\n"
        ).arg(hostname, timezone, keymap, rootPassword, userName, userPassword);

        // Handle bootloader installation
        if (bootloader == "GRUB") {
            chrootScript +=
            "# GRUB bootloader\n"
            "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=CachyOS\n"
            "grub-mkconfig -o /boot/grub/grub.cfg\n\n";
        } else if (bootloader == "systemd-boot") {
            chrootScript +=
            "# systemd-boot\n"
            "bootctl --path=/boot/efi install\n"
            "mkdir -p /boot/efi/loader/entries\n"
            "cat > /boot/efi/loader/loader.conf << 'LOADER'\n"
            "default arch\n"
            "timeout 3\n"
            "editor  yes\n"
            "LOADER\n\n"
            "cat > /boot/efi/loader/entries/arch.conf << 'ENTRY'\n"
            "title   CachyOS Linux\n"
            "linux   /vmlinuz-%7\n"
            "initrd  /initramfs-%7.img\n"
            "options root=UUID=%8 rootflags=subvol=@ rw\n"
            "ENTRY\n\n";
        } else if (bootloader == "rEFInd") {
            chrootScript +=
            "# rEFInd\n"
            "refind-install\n"
            "mkdir -p /boot/efi/EFI/refind/refind.conf\n"
            "cat > /boot/efi/EFI/refind/refind.conf << 'REFIND'\n"
            "menuentry \"CachyOS Linux\" {\n"
            "    icon     /EFI/refind/icons/os_arch.png\n"
            "    loader   /vmlinuz-%7\n"
            "    initrd   /initramfs-%7.img\n"
            "    options  \"root=UUID=%8 rootflags=subvol=@ rw\"\n"
            "}\n"
            "REFIND\n\n";
        }

        // Handle initramfs
        if (initramfs == "mkinitcpio") {
            chrootScript += "mkinitcpio -P\n\n";
        } else if (initramfs == "dracut") {
            chrootScript += "dracut --regenerate-all --force\n\n";
        } else if (initramfs == "booster") {
            chrootScript += "booster generate\n\n";
        } else if (initramfs == "mkinitcpio-pico") {
            chrootScript += "mkinitcpio -P\n\n";
        }

        // Network manager
        if (desktopEnv == "None") {
            chrootScript += "systemctl enable NetworkManager\n\n";
        }

        // Update package database
        chrootScript += "pacman -Sy\n\n";

        // Install desktop environment
        if (desktopEnv == "KDE Plasma") {
            chrootScript +=
            "# KDE Plasma\n"
            "pacstrap -i /mnt plasma-meta kde-applications-meta sddm cachyos-kde-settings --noconfirm --disable-download-timeout\n"
            "systemctl enable sddm\n"
            "pacstrap -i /mnt firefox dolphin konsole pulseaudio pavucontrol --noconfirm --disable-download-timeout\n";
        } else if (desktopEnv == "GNOME") {
            chrootScript +=
            "# GNOME\n"
            "pacstrap -i /mnt gnome gnome-extra gdm --noconfirm --disable-download-timeout\n"
            "systemctl enable gdm\n"
            "pacstrap -i /mnt firefox gnome-terminal pulseaudio pavucontrol --noconfirm --disable-download-timeout\n";
        } else if (desktopEnv == "XFCE") {
            chrootScript +=
            "# XFCE\n"
            "pacstrap -i /mnt xfce4 xfce4-goodies lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout\n"
            "systemctl enable lightdm\n"
            "pacstrap -i /mnt firefox mousepad xfce4-terminal pulseaudio pavucontrol --noconfirm --disable-download-timeout\n";
        } else if (desktopEnv == "MATE") {
            chrootScript +=
            "# MATE\n"
            "pacstrap -i /mnt mate mate-extra mate-media lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout\n"
            "systemctl enable lightdm\n"
            "pacstrap -i /mnt firefox pluma mate-terminal pulseaudio pavucontrol --noconfirm --disable-download-timeout\n";
        } else if (desktopEnv == "LXQt") {
            chrootScript +=
            "# LXQt\n"
            "pacstrap -i /mnt lxqt breeze-icons sddm --noconfirm --disable-download-timeout\n"
            "systemctl enable sddm\n"
            "pacstrap -i /mnt firefox qterminal pulseaudio pavucontrol --noconfirm --disable-download-timeout\n";
        } else if (desktopEnv == "Cinnamon") {
            chrootScript +=
            "# Cinnamon\n"
            "pacstrap -i /mnt cinnamon cinnamon-translations lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout\n"
            "systemctl enable lightdm\n"
            "pacstrap -i /mnt firefox xed gnome-terminal pulseaudio pavucontrol --noconfirm --disable-download-timeout\n";
        } else if (desktopEnv == "Budgie") {
            chrootScript +=
            "# Budgie\n"
            "pacstrap -i /mnt budgie-desktop budgie-extras gnome-control-center gnome-terminal lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout\n"
            "systemctl enable lightdm\n"
            "pacstrap -i /mnt firefox gnome-text-editor gnome-terminal pulseaudio pavucontrol --noconfirm --disable-download-timeout\n";
        } else if (desktopEnv == "Deepin") {
            chrootScript +=
            "# Deepin\n"
            "pacstrap -i /mnt deepin deepin-extra lightdm --noconfirm --disable-download-timeout\n"
            "systemctl enable lightdm\n"
            "pacstrap -i /mnt firefox deepin-terminal pulseaudio pavucontrol --noconfirm --disable-download-timeout\n";
        } else if (desktopEnv == "i3") {
            chrootScript +=
            "# i3\n"
            "pacstrap -i /mnt i3-wm i3status i3lock dmenu lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout\n"
            "systemctl enable lightdm\n"
            "pacstrap -i /mnt firefox alacritty pulseaudio pavucontrol --noconfirm --disable-download-timeout\n";
        } else if (desktopEnv == "Sway") {
            chrootScript +=
            "# Sway\n"
            "pacstrap -i /mnt sway swaylock swayidle waybar wofi lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout\n"
            "systemctl enable lightdm\n"
            "pacstrap -i /mnt firefox foot pulseaudio pavucontrol --noconfirm --disable-download-timeout\n";
        } else if (desktopEnv == "Hyprland") {
            chrootScript +=
            "# Hyprland\n"
            "pacstrap -i /mnt hyprland waybar rofi wofi kitty swaybg swaylock-effects wl-clipboard lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout\n"
            "systemctl enable lightdm\n"
            "pacstrap -i /mnt firefox kitty pulseaudio pavucontrol --noconfirm --disable-download-timeout\n"
            "mkdir -p /home/%5/.config/hypr\n"
            "cat > /home/%5/.config/hypr/hyprland.conf << 'HYPRCONFIG'\n"
            "# This is a basic Hyprland config\n"
            "exec-once = waybar &\n"
            "exec-once = swaybg -i ~/wallpaper.jpg &\n\n"
            "monitor=,preferred,auto,1\n\n"
            "input {\n"
            "    kb_layout = us\n"
            "    follow_mouse = 1\n"
            "    touchpad {\n"
            "        natural_scroll = yes\n"
            "    }\n"
            "}\n\n"
            "general {\n"
            "    gaps_in = 5\n"
            "    gaps_out = 10\n"
            "    border_size = 2\n"
            "    col.active_border = rgba(33ccffee) rgba(00ff99ee) 45deg\n"
            "    col.inactive_border = rgba(595959aa)\n"
            "}\n\n"
            "decoration {\n"
            "    rounding = 5\n"
            "    blur = yes\n"
            "    blur_size = 3\n"
            "    blur_passes = 1\n"
            "    blur_new_optimizations = on\n"
            "}\n\n"
            "animations {\n"
            "    enabled = yes\n"
            "    bezier = myBezier, 0.05, 0.9, 0.1, 1.05\n"
            "    animation = windows, 1, 7, myBezier\n"
            "    animation = windowsOut, 1, 7, default, popin 80%\n"
            "    animation = border, 1, 10, default\n"
            "    animation = fade, 1, 7, default\n"
            "    animation = workspaces, 1, 6, default\n"
            "}\n\n"
            "dwindle {\n"
            "    pseudotile = yes\n"
            "    preserve_split = yes\n"
            "}\n\n"
            "master {\n"
            "    new_is_master = true\n"
            "}\n\n"
            "bind = SUPER, Return, exec, kitty\n"
            "bind = SUPER, Q, killactive,\n"
            "bind = SUPER, M, exit,\n"
            "bind = SUPER, V, togglefloating,\n"
            "bind = SUPER, F, fullscreen,\n"
            "bind = SUPER, D, exec, rofi -show drun\n"
            "bind = SUPER, P, pseudo,\n"
            "bind = SUPER, J, togglesplit,\n"
            "HYPRCONFIG\n"
            "chown -R %5:%5 /home/%5/.config\n";
        }

        chrootScript +=
        "# Enable TRIM for SSDs\n"
        "systemctl enable fstrim.timer\n\n"
        "# Clean up\n"
        "rm /setup-chroot.sh\n";

        // Save and execute chroot script
        executeCommand(QString("echo \"%1\" > /mnt/setup-chroot.sh").arg(chrootScript));
        executeCommand("chmod +x /mnt/setup-chroot.sh");
        executeCommand("arch-chroot /mnt /setup-chroot.sh");
        progressBar->setValue(90);

        // Unmount
        executeCommand("umount -R /mnt");
        progressBar->setValue(100);
        appendToConsole(COLOR_CYAN + "Installation complete!" + COLOR_RESET);

        // Post-install options
        QStringList options = {"Reboot now", "Chroot into installed system", "Exit without rebooting"};
        bool ok;
        QString choice = QInputDialog::getItem(this, "Installation Complete",
                                               "Select post-install action:", options, 0, false, &ok);

        if (ok) {
            if (choice == "Reboot now") {
                executeCommand("reboot");
            } else if (choice == "Chroot into installed system") {
                executeCommand(QString("mount %11 /mnt/boot/efi").arg(targetDisk));
                executeCommand(QString("mount -o subvol=@ %12 /mnt").arg(targetDisk));
                executeCommand("mount -t proc none /mnt/proc");
                executeCommand("mount --rbind /dev /mnt/dev");
                executeCommand("mount --rbind /sys /mnt/sys");
                executeCommand("mount --rbind /dev/pts /mnt/dev/pts");
                executeCommand("arch-chroot /mnt /bin/bash");
                executeCommand("umount -R /mnt");
            }
        }
    }

    void executeCommand(const QString &command) {
        QString fullCommand = QString("echo '%1' | sudo -S %2").arg(sudoPassword, command);
        appendToConsole(COLOR_CYAN + "Executing: " + command + COLOR_RESET);

        QProcess process;
        process.start("bash", {"-c", fullCommand});
        process.waitForFinished(-1);

        QString output = process.readAllStandardOutput();
        QString error = process.readAllStandardError();

        if (!output.isEmpty()) appendToConsole(output);
        if (!error.isEmpty()) appendToConsole(COLOR_RED + error + COLOR_RESET);

        logViewer->appendLog("Command: " + command + "\n" + output + error);
    }

    void appendToConsole(const QString &text) {
        console->append(text);
        console->verticalScrollBar()->setValue(console->verticalScrollBar()->maximum());
        logViewer->appendLog(text);
    }

    void loadSettings() {
        QSettings settings("CachyOS", "Installer");
        targetDisk = settings.value("targetDisk").toString();
        hostname = settings.value("hostname").toString();
        timezone = settings.value("timezone").toString();
        keymap = settings.value("keymap").toString();
        userName = settings.value("userName").toString();
        userPassword = settings.value("userPassword").toString();
        rootPassword = settings.value("rootPassword").toString();
        kernelType = settings.value("kernelType").toString();
        initramfs = settings.value("initramfs").toString();
        bootloader = settings.value("bootloader").toString();
        repositories = settings.value("repositories").toStringList();
        desktopEnv = settings.value("desktopEnv").toString();
        compressionLevel = settings.value("compressionLevel").toInt();
    }

    void saveSettings() {
        QSettings settings("CachyOS", "Installer");
        settings.setValue("targetDisk", targetDisk);
        settings.setValue("hostname", hostname);
        settings.setValue("timezone", timezone);
        settings.setValue("keymap", keymap);
        settings.setValue("userName", userName);
        settings.setValue("userPassword", userPassword);
        settings.setValue("rootPassword", rootPassword);
        settings.setValue("kernelType", kernelType);
        settings.setValue("initramfs", initramfs);
        settings.setValue("bootloader", bootloader);
        settings.setValue("repositories", repositories);
        settings.setValue("desktopEnv", desktopEnv);
        settings.setValue("compressionLevel", compressionLevel);
    }

private:
    QLabel *asciiArt;
    QTextEdit *console;
    QProgressBar *progressBar;
    LogViewer *logViewer;
    QString sudoPassword;

    // Configuration variables
    QString targetDisk;
    QString hostname;
    QString timezone;
    QString keymap;
    QString userName;
    QString userPassword;
    QString rootPassword;
    QString kernelType;
    QString initramfs;
    QString bootloader;
    QStringList repositories;
    QString desktopEnv;
    int compressionLevel;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Check if running as root
    if (QProcess::execute("id", {"-u"}) != 0) {
        QMessageBox::critical(nullptr, "Error", "This application must be run as root!");
        return 1;
    }

    CachyOSInstaller installer;
    installer.show();

    return app.exec();
}

#include "main.moc"
