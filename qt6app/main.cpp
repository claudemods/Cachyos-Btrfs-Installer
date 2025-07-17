#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QProgressBar>
#include <QFile>
#include <QProcess>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QInputDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QScrollArea>
#include <QGroupBox>
#include <QSettings>
#include <QTimer>
#include <QThread>
#include <QLabel>
#include <QFutureWatcher>
#include <QtConcurrent>

class InstallerWindow : public QMainWindow {
    Q_OBJECT

public:
    InstallerWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("CachyOS Btrfs Installer v1.02 build 17-07-2025");
        resize(800, 600);

        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        // ASCII Art
        asciiArt = new QLabel(this);
        asciiArt->setTextInteractionFlags(Qt::TextSelectableByMouse);
        asciiArt->setStyleSheet("font-family: monospace; font-size: 10pt;");
        asciiArt->setText(
            "<span style='color: red;'>"
            "░█████╗░██╗░░░░░░█████╗░██║░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗<br>"
            "██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝<br>"
            "██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░<br>"
            "██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗<br>"
            "╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝<br>"
            "░╚════╝░╚══════╝╚═╝░░░░░░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░<br>"
            "</span>"
            "<span style='color: cyan;'>CachyOS Btrfs Installer v1.02 build 17-07-2025</span>"
        );
        mainLayout->addWidget(asciiArt);

        // Log display
        logDisplay = new QTextEdit(this);
        logDisplay->setReadOnly(true);
        logDisplay->setStyleSheet("font-family: monospace;");
        mainLayout->addWidget(logDisplay);

        // Progress bar
        progressBar = new QProgressBar(this);
        progressBar->setTextVisible(true);
        mainLayout->addWidget(progressBar);

        // Buttons
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        configureButton = new QPushButton("Configure", this);
        installButton = new QPushButton("Install", this);
        installButton->setEnabled(false);
        buttonLayout->addWidget(configureButton);
        buttonLayout->addWidget(installButton);
        mainLayout->addLayout(buttonLayout);

        setCentralWidget(centralWidget);

        connect(configureButton, &QPushButton::clicked, this, &InstallerWindow::configureInstallation);
        connect(installButton, &QPushButton::clicked, this, &InstallerWindow::startInstallation);

        initVariables();

        bool ok;
        sudoPassword = QInputDialog::getText(this, "Sudo Password",
                                             "Enter your sudo password:",
                                             QLineEdit::Password,
                                             "", &ok);
        if (!ok || sudoPassword.isEmpty()) {
            QTimer::singleShot(0, this, &QMainWindow::close);
        }
    }

private:
    QLabel *asciiArt;
    QTextEdit *logDisplay;
    QProgressBar *progressBar;
    QPushButton *configureButton;
    QPushButton *installButton;
    QString sudoPassword;

    QString TARGET_DISK;
    QString HOSTNAME;
    QString TIMEZONE;
    QString KEYMAP;
    QString USER_NAME;
    QString USER_PASSWORD;
    QString ROOT_PASSWORD;
    QString DESKTOP_ENV;
    QString KERNEL_TYPE;
    QString INITRAMFS;
    QString BOOTLOADER;
    QString BOOT_FS_TYPE = "fat32";
    QString LOCALE_LANG = "en_GB.UTF-8";
    QStringList REPOS;
    QStringList CUSTOM_PACKAGES;
    int COMPRESSION_LEVEL = 3;
    QFile logFile;
    QFutureWatcher<void> installationWatcher;

    void initVariables() {
        logFile.setFileName("installation_log.txt");
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            logMessage("Could not open log file for writing");
        }
    }

    void logMessage(const QString &message) {
        QString timestampedMsg = QString("[%1] %2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"), message);
        logDisplay->append(timestampedMsg);
        logFile.write(timestampedMsg.toUtf8() + "\n");
        logFile.flush();
        QApplication::processEvents();
    }

    QString executeCommand(const QString &cmd, bool useSudo = true, bool sensitive = false) {
        QProcess process;
        QString logCmd = sensitive ? "[REDACTED]" : cmd;
        logMessage("[EXEC] " + logCmd);

        process.setProcessChannelMode(QProcess::MergedChannels);
        process.setInputChannelMode(QProcess::InputChannelMode::ManagedInputChannel);

        if (useSudo) {
            process.start("sudo", QStringList() << "sh" << "-c" << cmd);
        } else {
            process.start("bash", QStringList() << "-c" << cmd);
        }

        // Process events periodically while waiting
        while (!process.waitForFinished(100)) {
            QApplication::processEvents();
        }

        if (process.exitCode() != 0) {
            QString error = QString(process.readAllStandardError());
            logMessage("Command failed: " + logCmd + "\nError: " + error);
            return "";
        }

        return QString(process.readAllStandardOutput()).trimmed();
    }

    bool fileExists(const QString &filename) {
        return QFile::exists(filename);
    }

    QStringList readFileLines(const QString &filename) {
        QStringList lines;
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty() && !line.startsWith("#")) {
                    lines.append(line);
                }
            }
        }
        return lines;
    }

    void loadConfigFile() {
        if (fileExists("installer.conf")) {
            logMessage("Loading configuration from installer.conf");
            QStringList lines = readFileLines("installer.conf");

            for (const QString &line : lines) {
                int pos = line.indexOf('=');
                if (pos != -1) {
                    QString key = line.left(pos);
                    QString value = line.mid(pos + 1);

                    if (key == "TARGET_DISK") TARGET_DISK = value;
                    else if (key == "HOSTNAME") HOSTNAME = value;
                    else if (key == "TIMEZONE") TIMEZONE = value;
                    else if (key == "KEYMAP") KEYMAP = value;
                    else if (key == "USER_NAME") USER_NAME = value;
                    else if (key == "USER_PASSWORD") USER_PASSWORD = value;
                    else if (key == "ROOT_PASSWORD") ROOT_PASSWORD = value;
                    else if (key == "DESKTOP_ENV") DESKTOP_ENV = value;
                    else if (key == "KERNEL_TYPE") KERNEL_TYPE = value;
                    else if (key == "INITRAMFS") INITRAMFS = value;
                    else if (key == "BOOTLOADER") BOOTLOADER = value;
                    else if (key == "BOOT_FS_TYPE") BOOT_FS_TYPE = value;
                    else if (key == "LOCALE_LANG") LOCALE_LANG = value;
                    else if (key == "COMPRESSION_LEVEL") COMPRESSION_LEVEL = value.toInt();
                }
            }
        }
    }

    void loadPackagesFile() {
        if (fileExists("packages.txt")) {
            logMessage("Loading additional packages from packages.txt");
            CUSTOM_PACKAGES = readFileLines("packages.txt");
        }
    }

    void configureInstallation() {
        loadConfigFile();

        if (TARGET_DISK.isEmpty()) {
            QStringList disks;
            QString output = executeCommand("lsblk -d -o NAME,SIZE -n");
            QStringList lines = output.split('\n');
            for (const QString &line : lines) {
                QStringList parts = line.split(' ');
                if (!parts.isEmpty()) {
                    disks << "/dev/" + parts[0];
                }
            }

            bool ok;
            TARGET_DISK = QInputDialog::getItem(this, "Target Disk",
                                                "Select target disk:",
                                                disks, 0, false, &ok);
            if (!ok || TARGET_DISK.isEmpty()) return;
        }

        if (BOOT_FS_TYPE.isEmpty()) {
            QStringList fsTypes;
            fsTypes << "fat32" << "ext4";
            bool ok;
            BOOT_FS_TYPE = QInputDialog::getItem(this, "Boot Filesystem",
                                                 "Select filesystem (Recommended: fat32 for UEFI):",
                                                 fsTypes, 0, false, &ok);
            if (!ok || BOOT_FS_TYPE.isEmpty()) return;
        }

        if (HOSTNAME.isEmpty()) {
            bool ok;
            HOSTNAME = QInputDialog::getText(this, "Hostname",
                                             "Enter hostname (e.g. cachyos):",
                                             QLineEdit::Normal,
                                             "", &ok);
            if (!ok || HOSTNAME.isEmpty()) return;
        }

        if (TIMEZONE.isEmpty()) {
            bool ok;
            TIMEZONE = QInputDialog::getText(this, "Timezone",
                                             "Enter timezone (e.g. Europe/London):",
                                             QLineEdit::Normal,
                                             "Europe/London", &ok);
            if (!ok || TIMEZONE.isEmpty()) return;
        }

        if (KEYMAP.isEmpty()) {
            bool ok;
            KEYMAP = QInputDialog::getText(this, "Keymap",
                                           "Enter keymap (e.g. uk):",
                                           QLineEdit::Normal,
                                           "us", &ok);
            if (!ok || KEYMAP.isEmpty()) return;
        }

        if (USER_NAME.isEmpty()) {
            bool ok;
            USER_NAME = QInputDialog::getText(this, "Username",
                                              "Enter username (lowercase, no spaces):",
                                              QLineEdit::Normal,
                                              "", &ok);
            if (!ok || USER_NAME.isEmpty()) return;
        }

        if (USER_PASSWORD.isEmpty()) {
            bool ok;
            USER_PASSWORD = QInputDialog::getText(this, "User Password",
                                                  "Enter password (min 4 chars):",
                                                  QLineEdit::Password,
                                                  "", &ok);
            if (!ok || USER_PASSWORD.isEmpty()) return;
        }

        if (ROOT_PASSWORD.isEmpty()) {
            bool ok;
            ROOT_PASSWORD = QInputDialog::getText(this, "Root Password",
                                                  "Enter root password (min 4 chars):",
                                                  QLineEdit::Password,
                                                  "", &ok);
            if (!ok || ROOT_PASSWORD.isEmpty()) return;
        }

        if (KERNEL_TYPE.isEmpty()) {
            QMap<QString, QString> kernels = {
                {"Bore", "CachyOS Bore"},
                {"Bore-Extra", "Bore with extras"},
                {"CachyOS", "Standard"},
                {"CachyOS-Extra", "With extras"},
                {"LTS", "Long-term"},
                {"Zen", "Zen kernel"}
            };

            bool ok;
            QStringList kernelOptions = kernels.keys();
            KERNEL_TYPE = QInputDialog::getItem(this, "Kernel",
                                                "Select kernel (Recommended: Bore for performance):",
                                                kernelOptions, 0, false, &ok);
            if (!ok || KERNEL_TYPE.isEmpty()) return;
        }

        if (INITRAMFS.isEmpty()) {
            QMap<QString, QString> initramfsOptions = {
                {"mkinitcpio", "Default"},
                {"dracut", "Alternative"},
                {"booster", "Fast"},
                {"mkinitcpio-pico", "Minimal"}
            };

            bool ok;
            QStringList options = initramfsOptions.keys();
            INITRAMFS = QInputDialog::getItem(this, "Initramfs",
                                              "Select initramfs (Recommended: mkinitcpio):",
                                              options, 0, false, &ok);
            if (!ok || INITRAMFS.isEmpty()) return;
        }

        if (BOOTLOADER.isEmpty()) {
            QMap<QString, QString> bootloaders = {
                {"GRUB", "GRUB"},
                {"systemd-boot", "Minimal"},
                {"rEFInd", "Graphical"}
            };

            bool ok;
            QStringList bootloaderOptions = bootloaders.keys();
            BOOTLOADER = QInputDialog::getItem(this, "Bootloader",
                                               "Select bootloader (Recommended: GRUB for flexibility):",
                                               bootloaderOptions, 0, false, &ok);
            if (!ok || BOOTLOADER.isEmpty()) return;
        }

        if (DESKTOP_ENV.isEmpty()) {
            QMap<QString, QString> desktops = {
                {"KDE Plasma", "KDE"},
                {"GNOME", "GNOME"},
                {"XFCE", "XFCE"},
                {"MATE", "MATE"},
                {"LXQt", "LXQt"},
                {"Cinnamon", "Cinnamon"},
                {"Budgie", "Budgie"},
                {"Deepin", "Deepin"},
                {"i3", "i3"},
                {"Sway", "Sway"},
                {"Hyprland", "Hyprland"},
                {"None", "None"}
            };

            bool ok;
            QStringList desktopOptions = desktops.keys();
            DESKTOP_ENV = QInputDialog::getItem(this, "Desktop",
                                                "Select desktop environment (Recommended: KDE Plasma):",
                                                desktopOptions, 0, false, &ok);
            if (!ok || DESKTOP_ENV.isEmpty()) return;
        }

        if (COMPRESSION_LEVEL == 0) {
            bool ok;
            COMPRESSION_LEVEL = QInputDialog::getInt(this, "Compression",
                                                     "Enter BTRFS compression level (1-22, Recommended: 3):",
                                                     3, 1, 22, 1, &ok);
            if (!ok) return;
        }

        if (LOCALE_LANG == "en_GB.UTF-8") {
            bool ok;
            LOCALE_LANG = QInputDialog::getText(this, "Locale",
                                                "Enter locale (e.g. en_GB.UTF-8):",
                                                QLineEdit::Normal,
                                                "en_GB.UTF-8", &ok);
            if (!ok || LOCALE_LANG.isEmpty()) return;
        }

        loadPackagesFile();

        QString summary = QString(
            "Installation Summary:\n"
            "----------------------\n"
            "Target Disk: %1\n"
            "Boot FS Type: %2\n"
            "Hostname: %3\n"
            "Timezone: %4\n"
            "Keymap: %5\n"
            "Username: %6\n"
            "Kernel: %7\n"
            "Initramfs: %8\n"
            "Bootloader: %9\n"
            "Desktop: %10\n"
            "Compression Level: %11\n"
            "Locale: %12\n"
            "Custom Packages: %13\n"
        ).arg(
            TARGET_DISK,
            BOOT_FS_TYPE,
            HOSTNAME,
            TIMEZONE,
            KEYMAP,
            USER_NAME,
            KERNEL_TYPE,
            INITRAMFS,
            BOOTLOADER,
            DESKTOP_ENV,
            QString::number(COMPRESSION_LEVEL),
            LOCALE_LANG,
            CUSTOM_PACKAGES.join(", ")
        );

        QMessageBox::information(this, "Configuration Summary", summary);
        installButton->setEnabled(true);
    }

    void setupLocaleConf() {
        QString localeConfPath = "/mnt/etc/locale.conf";
        QString content = QString(
            "LANG=%1\n"
            "LC_ADDRESS=%1\n"
            "LC_IDENTIFICATION=%1\n"
            "LC_MEASUREMENT=%1\n"
            "LC_MONETARY=%1\n"
            "LC_NAME=%1\n"
            "LC_NUMERIC=%1\n"
            "LC_PAPER=%1\n"
            "LC_TELEPHONE=%1\n"
            "LC_TIME=%1\n"
        ).arg(LOCALE_LANG);

        executeCommand(QString("mkdir -p /mnt/etc"));
        executeCommand(QString("echo '%1' > %2").arg(content, localeConfPath));
    }

    void startInstallation() {
        // Disable buttons during installation
        configureButton->setEnabled(false);
        installButton->setEnabled(false);

        // Run installation in background thread
        QFuture<void> future = QtConcurrent::run([this]() {
            performInstallation();
        });

        // Connect watcher to re-enable UI when done
        installationWatcher.setFuture(future);
        connect(&installationWatcher, &QFutureWatcher<void>::finished, this, [this]() {
            configureButton->setEnabled(true);
            installButton->setEnabled(true);
        });
    }

    void performInstallation() {
        logMessage("Starting installation process");

        // Check if running as root
        if (executeCommand("id -u") != "0") {
            QMetaObject::invokeMethod(this, [this]() {
                QMessageBox::critical(this, "Error", "Must be run as root!");
            }, Qt::QueuedConnection);
            return;
        }

        // Check UEFI
        if (!fileExists("/sys/firmware/efi")) {
            QMetaObject::invokeMethod(this, [this]() {
                QMessageBox::critical(this, "Error", "UEFI required!");
            }, Qt::QueuedConnection);
            return;
        }

        const int TOTAL_STEPS = 15;
        int current_step = 0;

        // Wipe disk
        logMessage("Wiping disk");
        executeCommand("wipefs -a " + TARGET_DISK);
        QMetaObject::invokeMethod(progressBar, "setValue", Q_ARG(int, ++current_step * 100 / TOTAL_STEPS));

        // Partition names
        QString boot_part = TARGET_DISK + (TARGET_DISK.contains("nvme") ? "p1" : "1");
        QString root_part = TARGET_DISK + (TARGET_DISK.contains("nvme") ? "p2" : "2");

        // Partitioning
        logMessage("Partitioning disk");
        executeCommand("parted -s " + TARGET_DISK + " mklabel gpt");
        executeCommand("parted -s " + TARGET_DISK + " mkpart primary 1MiB 513MiB");
        executeCommand("parted -s " + TARGET_DISK + " set 1 esp on");
        executeCommand("parted -s " + TARGET_DISK + " mkpart primary 513MiB 100%");
        QMetaObject::invokeMethod(progressBar, "setValue", Q_ARG(int, ++current_step * 100 / TOTAL_STEPS));

        // Formatting
        logMessage("Formatting partitions");
        if (BOOT_FS_TYPE == "fat32") {
            executeCommand("mkfs.vfat -F32 " + boot_part);
        } else {
            executeCommand("mkfs.ext4 " + boot_part);
        }
        executeCommand("mkfs.btrfs -f " + root_part);
        QMetaObject::invokeMethod(progressBar, "setValue", Q_ARG(int, ++current_step * 100 / TOTAL_STEPS));

        // Mounting and subvolumes
        logMessage("Setting up Btrfs subvolumes");
        executeCommand("mount " + root_part + " /mnt");
        executeCommand("btrfs subvolume create /mnt/@");
        executeCommand("btrfs subvolume create /mnt/@home");
        executeCommand("btrfs subvolume create /mnt/@root");
        executeCommand("btrfs subvolume create /mnt/@srv");
        executeCommand("btrfs subvolume create /mnt/@cache");
        executeCommand("btrfs subvolume create /mnt/@tmp");
        executeCommand("btrfs subvolume create /mnt/@log");
        executeCommand("umount /mnt");
        QMetaObject::invokeMethod(progressBar, "setValue", Q_ARG(int, ++current_step * 100 / TOTAL_STEPS));

        // Remount with compression
        logMessage("Mounting with compression");
        executeCommand("mount -o subvol=@,compress=zstd:" + QString::number(COMPRESSION_LEVEL) + ",compress-force=zstd:" + QString::number(COMPRESSION_LEVEL) + " " + root_part + " /mnt");
        executeCommand("mkdir -p /mnt/boot/efi");
        executeCommand("mount " + boot_part + " /mnt/boot/efi");
        executeCommand("mkdir -p /mnt/home");
        executeCommand("mkdir -p /mnt/root");
        executeCommand("mkdir -p /mnt/srv");
        executeCommand("mkdir -p /mnt/tmp");
        executeCommand("mkdir -p /mnt/var/cache");
        executeCommand("mkdir -p /mnt/var/log");
        executeCommand("mount -o subvol=@home,compress=zstd:" + QString::number(COMPRESSION_LEVEL) + ",compress-force=zstd:" + QString::number(COMPRESSION_LEVEL) + " " + root_part + " /mnt/home");
        executeCommand("mount -o subvol=@root,compress=zstd:" + QString::number(COMPRESSION_LEVEL) + ",compress-force=zstd:" + QString::number(COMPRESSION_LEVEL) + " " + root_part + " /mnt/root");
        executeCommand("mount -o subvol=@srv,compress=zstd:" + QString::number(COMPRESSION_LEVEL) + ",compress-force=zstd:" + QString::number(COMPRESSION_LEVEL) + " " + root_part + " /mnt/srv");
        executeCommand("mount -o subvol=@tmp,compress=zstd:" + QString::number(COMPRESSION_LEVEL) + ",compress-force=zstd:" + QString::number(COMPRESSION_LEVEL) + " " + root_part + " /mnt/tmp");
        executeCommand("mount -o subvol=@cache,compress=zstd:" + QString::number(COMPRESSION_LEVEL) + ",compress-force=zstd:" + QString::number(COMPRESSION_LEVEL) + " " + root_part + " /mnt/var/cache");
        executeCommand("mount -o subvol=@log,compress=zstd:" + QString::number(COMPRESSION_LEVEL) + ",compress-force=zstd:" + QString::number(COMPRESSION_LEVEL) + " " + root_part + " /mnt/var/log");
        QMetaObject::invokeMethod(progressBar, "setValue", Q_ARG(int, ++current_step * 100 / TOTAL_STEPS));

        // Kernel package
        QString KERNEL_PKG;
        if (KERNEL_TYPE == "Bore") KERNEL_PKG = "linux-cachyos-bore";
        else if (KERNEL_TYPE == "Bore-Extra") KERNEL_PKG = "linux-cachyos-bore-extra";
        else if (KERNEL_TYPE == "CachyOS") KERNEL_PKG = "linux-cachyos";
        else if (KERNEL_TYPE == "CachyOS-Extra") KERNEL_PKG = "linux-cachyos-extra";
        else if (KERNEL_TYPE == "LTS") KERNEL_PKG = "linux-lts";
        else if (KERNEL_TYPE == "Zen") KERNEL_PKG = "linux-zen";

        // Base packages
        QString BASE_PKGS = "base " + KERNEL_PKG + " linux-firmware sudo dosfstools arch-install-scripts btrfs-progs nano --needed --disable-download-timeout";

        // Add custom packages if any
        if (!CUSTOM_PACKAGES.isEmpty()) {
            BASE_PKGS += " " + CUSTOM_PACKAGES.join(" ");
        }

        if (BOOTLOADER == "GRUB") {
            BASE_PKGS += " grub efibootmgr dosfstools cachyos-grub-theme --needed --disable-download-timeout";
        } else if (BOOTLOADER == "systemd-boot") {
            BASE_PKGS += " efibootmgr --needed --disable-download-timeout";
        } else if (BOOTLOADER == "rEFInd") {
            BASE_PKGS += " refind --needed --disable-download-timeout";
        }

        if (INITRAMFS == "mkinitcpio") {
            BASE_PKGS += " mkinitcpio --needed";
        } else if (INITRAMFS == "dracut") {
            BASE_PKGS += " dracut --needed";
        } else if (INITRAMFS == "booster") {
            BASE_PKGS += " booster --needed";
        } else if (INITRAMFS == "mkinitcpio-pico") {
            BASE_PKGS += " mkinitcpio-pico --needed";
        }

        if (DESKTOP_ENV == "None") {
            BASE_PKGS += " networkmanager --needed --disable-download-timeout";
        }

        // Base system installation
        logMessage("Installing base system");
        executeCommand("pacstrap -i /mnt " + BASE_PKGS);
        executeCommand("tar -xvzf pacman-16-07-2025.tar.gz -C /mnt/etc");
        QMetaObject::invokeMethod(progressBar, "setValue", Q_ARG(int, ++current_step * 100 / TOTAL_STEPS));

        // Generate fstab
        logMessage("Generating fstab");
        QString ROOT_UUID = executeCommand("blkid -s UUID -o value " + root_part);
        QString fstabContent = QString(
            "\n# Btrfs subvolumes\n"
            "UUID=%1 / btrfs rw,noatime,compress=zstd:%2,subvol=@ 0 0\n"
            "UUID=%1 /home btrfs rw,noatime,compress=zstd:%2,subvol=@home 0 0\n"
            "UUID=%1 /root btrfs rw,noatime,compress=zstd:%2,subvol=@root 0 0\n"
            "UUID=%1 /srv btrfs rw,noatime,compress=zstd:%2,subvol=@srv 0 0\n"
            "UUID=%1 /var/cache btrfs rw,noatime,compress=zstd:%2,subvol=@cache 0 0\n"
            "UUID=%1 /var/tmp btrfs rw,noatime,compress=zstd:%2,subvol=@tmp 0 0\n"
            "UUID=%1 /var/log btrfs rw,noatime,compress=zstd:%2,subvol=@log 0 0\n"
        ).arg(ROOT_UUID, QString::number(COMPRESSION_LEVEL));

        executeCommand(QString("echo '%1' >> /mnt/etc/fstab").arg(fstabContent));
        QMetaObject::invokeMethod(progressBar, "setValue", Q_ARG(int, ++current_step * 100 / TOTAL_STEPS));

        // Setup locale
        logMessage("Configuring locale");
        setupLocaleConf();
        QMetaObject::invokeMethod(progressBar, "setValue", Q_ARG(int, ++current_step * 100 / TOTAL_STEPS));

        // Chroot setup
        logMessage("Preparing chroot environment");
        QString chrootScript = QString(
            "#!/bin/bash\n"
            "# System config\n"
            "echo \"%1\" > /etc/hostname\n"
            "ln -sf /usr/share/zoneinfo/%2 /etc/localtime\n"
            "hwclock --systohc\n"
            "echo \"%3\" >> /etc/locale.gen\n"
            "locale-gen\n\n"
            "# Users\n"
            "echo \"root:%4\" | chpasswd\n"
            "useradd -m -G wheel,audio,video,storage,optical -s /bin/bash \"%5\"\n"
            "echo \"%5:%6\" | chpasswd\n"
            "echo \"%%wheel ALL=(ALL) ALL\" > /etc/sudoers.d/wheel\n\n"
        ).arg(
            HOSTNAME,
            TIMEZONE,
            LOCALE_LANG,
            ROOT_PASSWORD,
            USER_NAME,
            USER_PASSWORD
        );

        if (BOOTLOADER == "GRUB") {
            chrootScript += QString(
                "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=CachyOS\n"
                "grub-mkconfig -o /boot/grub/grub.cfg\n"
            );
        } else if (BOOTLOADER == "systemd-boot") {
            chrootScript += QString(
                "bootctl --path=/boot/efi install\n"
                "mkdir -p /boot/efi/loader/entries\n"
                "cat > /boot/efi/loader/loader.conf << 'LOADER'\n"
                "default arch\n"
                "timeout 3\n"
                "editor  yes\n"
                "LOADER\n\n"
                "cat > /boot/efi/loader/entries/arch.conf << 'ENTRY'\n"
                "title   CachyOS Linux\n"
                "linux   /vmlinuz-%1\n"
                "initrd  /initramfs-%1.img\n"
                "options root=UUID=%2 rootflags=subvol=@ rw\n"
                "ENTRY\n"
            ).arg(KERNEL_PKG, ROOT_UUID);
        } else if (BOOTLOADER == "rEFInd") {
            chrootScript += QString(
                "refind-install\n"
                "mkdir -p /boot/efi/EFI/refind/refind.conf\n"
                "cat > /boot/efi/EFI/refind/refind.conf << 'REFIND'\n"
                "menuentry \"CachyOS Linux\" {\n"
                "    icon     /EFI/refind/icons/os_arch.png\n"
                "    loader   /vmlinuz-%1\n"
                "    initrd   /initramfs-%1.img\n"
                "    options  \"root=UUID=%2 rootflags=subvol=@ rw\"\n"
                "}\n"
                "REFIND\n"
            ).arg(KERNEL_PKG, ROOT_UUID);
        }

        chrootScript += "\n# Initramfs\n";
        if (INITRAMFS == "mkinitcpio") {
            chrootScript += "mkinitcpio -P\n";
        } else if (INITRAMFS == "dracut") {
            chrootScript += "dracut --regenerate-all --force\n";
        } else if (INITRAMFS == "booster") {
            chrootScript += "booster generate\n";
        } else if (INITRAMFS == "mkinitcpio-pico") {
            chrootScript += "mkinitcpio -P\n";
        }

        chrootScript += "\n# Network\n";
        if (DESKTOP_ENV == "None") {
            chrootScript += "systemctl enable NetworkManager\n";
            chrootScript += "systemctl start NetworkManager\n";
        }

        chrootScript += "\n# Desktop environments\n";
        if (DESKTOP_ENV != "None") {
            chrootScript += "pacman -S --noconfirm --needed --disable-download-timeout ";
        }

        if (DESKTOP_ENV == "KDE Plasma") {
            chrootScript += QString(
                "plasma-desktop qt6-base qt6-wayland wayland kde-applications-meta sddm cachyos-kde-settings ntfs-3g gtk3\n"
                "systemctl enable sddm\n"
                "systemctl enable NetworkManager\n"
                "systemctl start NetworkManager\n"
                "echo 'blacklist ntfs3' | tee /etc/modprobe.d/disable-ntfs3.conf\n"
                "pacman -S --noconfirm --needed --disable-download-timeout firefox kate ksystemlog partitionmanager dolphin konsole pulseaudio pavucontrol\n"
                "plymouth-set-default-theme -R cachyos-bootanimation\n"
                "if [ -f /setup-chroot-gaming ]; then\n"
                "    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta\n"
                "fi\n"
            );
        } else if (DESKTOP_ENV == "GNOME") {
            chrootScript += QString(
                "gnome gnome-extra gdm\n"
                "systemctl enable gdm\n"
                "systemctl enable NetworkManager\n"
                "systemctl start NetworkManager\n"
                "pacman -S --noconfirm --needed --disable-download-timeout firefox gnome-terminal pulseaudio pavucontrol\n"
                "if [ -f /setup-chroot-gaming ]; then\n"
                "    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta\n"
                "fi\n"
            );
        } else if (DESKTOP_ENV == "XFCE") {
            chrootScript += QString(
                "xfce4 xfce4-goodies lightdm lightdm-gtk-greeter\n"
                "systemctl enable lightdm\n"
                "systemctl enable NetworkManager\n"
                "systemctl start NetworkManager\n"
                "pacman -S --noconfirm --needed --disable-download-timeout firefox mousepad xfce4-terminal pulseaudio pavucontrol\n"
                "if [ -f /setup-chroot-gaming ]; then\n"
                "    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta\n"
                "fi\n"
            );
        } else if (DESKTOP_ENV == "MATE") {
            chrootScript += QString(
                "mate mate-extra mate-media lightdm lightdm-gtk-greeter\n"
                "systemctl enable lightdm\n"
                "systemctl enable NetworkManager\n"
                "systemctl start NetworkManager\n"
                "pacman -S --noconfirm --needed --disable-download-timeout firefox pluma mate-terminal pulseaudio pavucontrol\n"
                "if [ -f /setup-chroot-gaming ]; then\n"
                "    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta\n"
                "fi\n"
            );
        } else if (DESKTOP_ENV == "LXQt") {
            chrootScript += QString(
                "lxqt breeze-icons sddm\n"
                "systemctl enable sddm\n"
                "systemctl enable NetworkManager\n"
                "systemctl start NetworkManager\n"
                "pacman -S --noconfirm --needed --disable-download-timeout firefox qterminal pulseaudio pavucontrol\n"
                "if [ -f /setup-chroot-gaming ]; then\n"
                "    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta\n"
                "fi\n"
            );
        } else if (DESKTOP_ENV == "Cinnamon") {
            chrootScript += QString(
                "cinnamon cinnamon-translations lightdm lightdm-gtk-greeter\n"
                "systemctl enable lightdm\n"
                "systemctl enable NetworkManager\n"
                "systemctl start NetworkManager\n"
                "pacman -S --noconfirm --needed --disable-download-timeout firefox xed gnome-terminal pulseaudio pavucontrol\n"
                "if [ -f /setup-chroot-gaming ]; then\n"
                "    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta\n"
                "fi\n"
            );
        } else if (DESKTOP_ENV == "Budgie") {
            chrootScript += QString(
                "budgie-desktop budgie-extras gnome-control-center gnome-terminal lightdm lightdm-gtk-greeter\n"
                "systemctl enable lightdm\n"
                "systemctl enable NetworkManager\n"
                "systemctl start NetworkManager\n"
                "pacman -S --noconfirm --needed --disable-download-timeout firefox gnome-text-editor gnome-terminal pulseaudio pavucontrol\n"
                "if [ -f /setup-chroot-gaming ]; then\n"
                "    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta\n"
                "fi\n"
            );
        } else if (DESKTOP_ENV == "Deepin") {
            chrootScript += QString(
                "deepin deepin-extra lightdm\n"
                "systemctl enable lightdm\n"
                "systemctl enable NetworkManager\n"
                "systemctl start NetworkManager\n"
                "pacman -S --noconfirm --needed --disable-download-timeout firefox deepin-terminal pulseaudio pavucontrol\n"
                "if [ -f /setup-chroot-gaming ]; then\n"
                "    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta\n"
                "fi\n"
            );
        } else if (DESKTOP_ENV == "i3") {
            chrootScript += QString(
                "i3-wm i3status i3lock dmenu lightdm lightdm-gtk-greeter\n"
                "systemctl enable lightdm\n"
                "systemctl enable NetworkManager\n"
                "systemctl start NetworkManager\n"
                "pacman -S --noconfirm --needed --disable-download-timeout firefox alacritty pulseaudio pavucontrol\n"
                "if [ -f /setup-chroot-gaming ]; then\n"
                "    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta\n"
                "fi\n"
            );
        } else if (DESKTOP_ENV == "Sway") {
            chrootScript += QString(
                "sway swaylock swayidle waybar wofi lightdm lightdm-gtk-greeter\n"
                "systemctl enable lightdm\n"
                "systemctl enable NetworkManager\n"
                "systemctl start NetworkManager\n"
                "pacman -S --noconfirm --needed --disable-download-timeout firefox foot pulseaudio pavucontrol\n"
                "if [ -f /setup-chroot-gaming ]; then\n"
                "    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta\n"
                "fi\n"
            );
        } else if (DESKTOP_ENV == "Hyprland") {
            chrootScript += QString(
                "hyprland waybar rofi wofi kitty swaybg swaylock-effects wl-clipboard lightdm lightdm-gtk-greeter\n"
                "systemctl enable lightdm\n"
                "systemctl enable NetworkManager\n"
                "systemctl start NetworkManager\n"
                "pacman -S --noconfirm --needed --disable-download-timeout firefox kitty pulseaudio pavucontrol\n"
                "if [ -f /setup-chroot-gaming ]; then\n"
                "    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta\n"
                "fi\n\n"
                "# Hyprland config\n"
                "mkdir -p /home/%1/.config/hypr\n"
                "cat > /home/%1/.config/hypr/hyprland.conf << 'HYPRCONFIG'\n"
                "exec-once = waybar &\n"
                "exec-once = swaybg -i ~/wallpaper.jpg &\n"
                "monitor=,preferred,auto,1\n"
                "input {\n"
                "    kb_layout = us\n"
                "    follow_mouse = 1\n"
                "    touchpad { natural_scroll = yes }\n"
                "}\n"
                "general {\n"
                "    gaps_in = 5\n"
                "    gaps_out = 10\n"
                "    border_size = 2\n"
                "    col.active_border = rgba(33ccffee) rgba(00ff99ee) 45deg\n"
                "    col.inactive_border = rgba(595959aa)\n"
                "}\n"
                "decoration {\n"
                "    rounding = 5\n"
                "    blur = yes\n"
                "    blur_size = 3\n"
                "    blur_passes = 1\n"
                "}\n"
                "animations {\n"
                "    enabled = yes\n"
                "    bezier = myBezier, 0.05, 0.9, 0.1, 1.05\n"
                "    animation = windows, 1, 7, myBezier\n"
                "    animation = windowsOut, 1, 7, default, popin 80%%\n"
                "    animation = border, 1, 10, default\n"
                "    animation = fade, 1, 7, default\n"
                "    animation = workspaces, 1, 6, default\n"
                "}\n"
                "bind = SUPER, Return, exec, kitty\n"
                "bind = SUPER, Q, killactive\n"
                "bind = SUPER, M, exit\n"
                "bind = SUPER, V, togglefloating\n"
                "bind = SUPER, F, fullscreen\n"
                "bind = SUPER, D, exec, rofi -show drun\n"
                "bind = SUPER, P, pseudo\n"
                "bind = SUPER, J, togglesplit\n"
                "HYPRCONFIG\n"
                "chown -R %1:%1 /home/%1/.config\n"
            ).arg(USER_NAME);
        }

        chrootScript += QString(
            "\n# Clean up\n"
            "rm /setup-chroot.sh\n"
            "if [ -f /setup-chroot-gaming ]; then\n"
            "    rm /setup-chroot-gaming\n"
            "fi\n"
        );

        // Check if gaming packages should be installed
        bool installGaming = false;
        QMetaObject::invokeMethod(this, [this, &installGaming]() {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, "Gaming Packages",
                "Install cachyos-gaming-meta package?",
                QMessageBox::Yes|QMessageBox::No
            );
            installGaming = (reply == QMessageBox::Yes);
        }, Qt::BlockingQueuedConnection);

        if (installGaming) {
            executeCommand("touch /mnt/setup-chroot-gaming");
        }

        executeCommand(QString("echo '%1' > /mnt/setup-chroot.sh").arg(chrootScript));
        executeCommand("chmod +x /mnt/setup-chroot.sh");
        logMessage("Running chroot configuration");
        executeCommand("arch-chroot /mnt /setup-chroot.sh");
        QMetaObject::invokeMethod(progressBar, "setValue", Q_ARG(int, ++current_step * 100 / TOTAL_STEPS));

        // Final cleanup
        logMessage("Finalizing installation");
        executeCommand("umount -R /mnt");
        QMetaObject::invokeMethod(progressBar, "setValue", Q_ARG(int, 100));

        logMessage("Installation complete!");
        QMetaObject::invokeMethod(this, [this]() {
            QMessageBox::information(this, "Success", "Installation complete!\nYou can now reboot into your new CachyOS installation.");
        }, Qt::QueuedConnection);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set application style and palette
    app.setStyle("Fusion");

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(53,53,53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(25,25,25));
    palette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53,53,53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(palette);

    InstallerWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"
