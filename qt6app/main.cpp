#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QPalette>
#include <QPixmap>
#include <QStringList>
#include <QTimer>
#include <QFont>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QDateTime>
#include <QTextStream>
#include <QSettings>
#include <QScrollArea>
#include <QFormLayout>
#include <QButtonGroup>

class InstallerWindow : public QMainWindow {
    Q_OBJECT
public:
    InstallerWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("CachyOS Btrfs Installer");
        resize(900, 700);

        // Set color scheme
        QPalette palette;
        palette.setColor(QPalette::Window, QColor("#00568f"));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(53, 53, 53));
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, QColor(255, 215, 0));
        setPalette(palette);

        // Create main widgets
        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        // ASCII Art Header
        QLabel *asciiArt = new QLabel(this);
        asciiArt->setText(
            "<span style='color:#ff0000; font-family:monospace;'>"
            "░█████╗░██╗░░░░░░█████╗░██║░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗<br>"
            "██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝<br>"
            "██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░<br>"
            "██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗<br>"
            "╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝<br>"
            "░╚════╝░╚══════╝╚═╝░░░░░░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░<br>"
            "<span style='color:#00ffff;'>CachyOS Btrfs Installer v1.2</span></span>"
        );
        asciiArt->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(asciiArt);

        // Configuration form
        createConfigForm();
        mainLayout->addWidget(configGroup);

        // Output console
        outputText = new QTextEdit(this);
        outputText->setReadOnly(true);
        outputText->setStyleSheet(
            "QTextEdit {"
            "background-color: black;"
            "color: lime;"
            "font-family: monospace;"
            "border: 2px solid gold;"
            "}"
        );
        mainLayout->addWidget(outputText);

        // Progress bar
        progressBar = new QProgressBar(this);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        progressBar->setStyleSheet(
            "QProgressBar {"
            "border: 2px solid grey;"
            "border-radius: 5px;"
            "text-align: center;"
            "background: #333333;"
            "}"
            "QProgressBar::chunk {"
            "background-color: gold;"
            "}"
        );
        mainLayout->addWidget(progressBar);

        // Buttons
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        startButton = new QPushButton("Start Installation", this);
        startButton->setStyleSheet(
            "QPushButton {"
            "background-color: #333333;"
            "color: gold;"
            "border: 2px solid gold;"
            "padding: 5px;"
            "}"
            "QPushButton:hover {"
            "background-color: #555555;"
            "}"
        );
        connect(startButton, &QPushButton::clicked, this, &InstallerWindow::startInstallation);
        buttonLayout->addWidget(startButton);

        quitButton = new QPushButton("Quit", this);
        quitButton->setStyleSheet(startButton->styleSheet());
        connect(quitButton, &QPushButton::clicked, qApp, &QApplication::quit);
        buttonLayout->addWidget(quitButton);

        mainLayout->addLayout(buttonLayout);

        setCentralWidget(centralWidget);

        // Initialize log file
        logFile.setFileName("installation_log.txt");
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            logMessage("Could not open log file!");
        }
    }

private slots:
    void startInstallation() {
        // Validate inputs
        if (targetDiskCombo->currentText().isEmpty()) {
            QMessageBox::warning(this, "Error", "Please select a target disk");
            return;
        }

        // Disable UI during installation
        startButton->setEnabled(false);
        quitButton->setEnabled(false);
        configGroup->setEnabled(false);

        // Start installation
        QTimer::singleShot(0, this, &InstallerWindow::performInstallation);
    }

private:
    void createConfigForm() {
        configGroup = new QGroupBox("Installation Configuration", this);
        QFormLayout *formLayout = new QFormLayout(configGroup);

        // Target Disk
        targetDiskCombo = new QComboBox(this);
        populateDisks();
        formLayout->addRow("Target Disk:", targetDiskCombo);

        // Hostname
        hostnameEdit = new QLineEdit("cachyos", this);
        formLayout->addRow("Hostname:", hostnameEdit);

        // Timezone
        timezoneEdit = new QLineEdit("Europe/London", this);
        formLayout->addRow("Timezone:", timezoneEdit);

        // Keymap
        keymapEdit = new QLineEdit("uk", this);
        formLayout->addRow("Keymap:", keymapEdit);

        // Username
        usernameEdit = new QLineEdit("user", this);
        formLayout->addRow("Username:", usernameEdit);

        // User Password
        userPasswordEdit = new QLineEdit(this);
        userPasswordEdit->setEchoMode(QLineEdit::Password);
        formLayout->addRow("User Password:", userPasswordEdit);

        // Root Password
        rootPasswordEdit = new QLineEdit(this);
        rootPasswordEdit->setEchoMode(QLineEdit::Password);
        formLayout->addRow("Root Password:", rootPasswordEdit);

        // Kernel
        kernelCombo = new QComboBox(this);
        kernelCombo->addItems({"Bore", "Bore-Extra", "CachyOS", "CachyOS-Extra", "LTS", "Zen"});
        kernelCombo->setCurrentIndex(0);
        formLayout->addRow("Kernel:", kernelCombo);

        // Initramfs
        initramfsCombo = new QComboBox(this);
        initramfsCombo->addItems({"mkinitcpio", "dracut", "booster", "mkinitcpio-pico"});
        initramfsCombo->setCurrentIndex(0);
        formLayout->addRow("Initramfs:", initramfsCombo);

        // Bootloader
        bootloaderCombo = new QComboBox(this);
        bootloaderCombo->addItems({"GRUB", "systemd-boot", "rEFInd"});
        bootloaderCombo->setCurrentIndex(0);
        formLayout->addRow("Bootloader:", bootloaderCombo);

        // Desktop Environment
        desktopCombo = new QComboBox(this);
        desktopCombo->addItems({"KDE Plasma", "GNOME", "XFCE", "MATE", "LXQt", "Cinnamon", "Budgie", "Deepin", "i3", "Sway", "Hyprland", "None"});
        desktopCombo->setCurrentIndex(0);
        formLayout->addRow("Desktop Environment:", desktopCombo);

        // Compression Level
        compressionSpin = new QLineEdit("3", this);
        formLayout->addRow("Btrfs Compression Level (1-22):", compressionSpin);

        // Locale
        localeEdit = new QLineEdit("en_GB.UTF-8", this);
        formLayout->addRow("Locale:", localeEdit);

        // Load saved config
        loadConfig();
    }

    void populateDisks() {
        QProcess lsblk;
        lsblk.start("lsblk", QStringList() << "-d" << "-o" << "NAME,SIZE" << "-n" << "-l");
        if (lsblk.waitForFinished()) {
            QString output = lsblk.readAllStandardOutput();
            foreach (const QString &line, output.split('\n', Qt::SkipEmptyParts)) {
                QStringList parts = line.trimmed().split(' ');
                if (parts.size() >= 2 && !parts[0].startsWith("loop")) {
                    targetDiskCombo->addItem("/dev/" + parts[0] + " (" + parts[1] + ")");
                }
            }
        }
    }

    void loadConfig() {
        QSettings settings("CachyOS", "Installer");
        targetDiskCombo->setCurrentText(settings.value("targetDisk").toString());
        hostnameEdit->setText(settings.value("hostname", "cachyos").toString());
        timezoneEdit->setText(settings.value("timezone", "Europe/London").toString());
        keymapEdit->setText(settings.value("keymap", "uk").toString());
        usernameEdit->setText(settings.value("username", "user").toString());
        kernelCombo->setCurrentText(settings.value("kernel", "Bore").toString());
        initramfsCombo->setCurrentText(settings.value("initramfs", "mkinitcpio").toString());
        bootloaderCombo->setCurrentText(settings.value("bootloader", "GRUB").toString());
        desktopCombo->setCurrentText(settings.value("desktop", "KDE Plasma").toString());
        compressionSpin->setText(settings.value("compression", "3").toString());
        localeEdit->setText(settings.value("locale", "en_GB.UTF-8").toString());
    }

    void saveConfig() {
        QSettings settings("CachyOS", "Installer");
        settings.setValue("targetDisk", targetDiskCombo->currentText());
        settings.setValue("hostname", hostnameEdit->text());
        settings.setValue("timezone", timezoneEdit->text());
        settings.setValue("keymap", keymapEdit->text());
        settings.setValue("username", usernameEdit->text());
        settings.setValue("kernel", kernelCombo->currentText());
        settings.setValue("initramfs", initramfsCombo->currentText());
        settings.setValue("bootloader", bootloaderCombo->currentText());
        settings.setValue("desktop", desktopCombo->currentText());
        settings.setValue("compression", compressionSpin->text());
        settings.setValue("locale", localeEdit->text());
    }

    void logMessage(const QString &message) {
        QString timestamped = QString("[%1] %2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), message);
        outputText->append(timestamped);
        QTextStream out(&logFile);
        out << timestamped << "\n";
        outputText->verticalScrollBar()->setValue(outputText->verticalScrollBar()->maximum());
    }

    void executeCommand(const QString &cmd) {
        logMessage("[EXEC] " + cmd);
        QProcess process;
        process.start("bash", QStringList() << "-c" << cmd);
        if (!process.waitForFinished(-1)) {
            logMessage("Error executing: " + cmd);
            QMessageBox::critical(this, "Error", "Command failed: " + cmd);
            return;
        }
        logMessage(process.readAllStandardOutput());
        if (process.exitCode() != 0) {
            logMessage("Command failed with exit code: " + QString::number(process.exitCode()));
            logMessage(process.readAllStandardError());
        }
    }

    void performInstallation() {
        const int TOTAL_STEPS = 15;
        int currentStep = 0;

        // Extract disk name from combo box (remove size info)
        QString targetDisk = targetDiskCombo->currentText().split(' ').first();

        // Wipe disk
        logMessage("Wiping disk");
        executeCommand("sudo wipefs -a " + targetDisk);
        progressBar->setValue(++currentStep * 100 / TOTAL_STEPS);

        // Partition names
        QString bootPart = targetDisk + (targetDisk.contains("nvme") ? "p1" : "1");
        QString rootPart = targetDisk + (targetDisk.contains("nvme") ? "p2" : "2");

        // Partitioning
        logMessage("Partitioning disk");
        executeCommand("sudo parted -s " + targetDisk + " mklabel gpt");
        executeCommand("sudo parted -s " + targetDisk + " mkpart primary 1MiB 513MiB");
        executeCommand("sudo parted -s " + targetDisk + " set 1 esp on");
        executeCommand("sudo parted -s " + targetDisk + " mkpart primary 513MiB 100%");
        progressBar->setValue(++currentStep * 100 / TOTAL_STEPS);

        // Formatting
        logMessage("Formatting partitions");
        executeCommand("sudo mkfs.vfat -F32 " + bootPart);
        executeCommand("sudo mkfs.btrfs -f " + rootPart);
        progressBar->setValue(++currentStep * 100 / TOTAL_STEPS);

        // Mounting and subvolumes
        logMessage("Setting up Btrfs subvolumes");
        executeCommand("sudo mount " + rootPart + " /mnt");
        executeCommand("sudo btrfs subvolume create /mnt/@");
        executeCommand("sudo btrfs subvolume create /mnt/@home");
        executeCommand("sudo btrfs subvolume create /mnt/@root");
        executeCommand("sudo btrfs subvolume create /mnt/@srv");
        executeCommand("sudo btrfs subvolume create /mnt/@cache");
        executeCommand("sudo btrfs subvolume create /mnt/@tmp");
        executeCommand("sudo btrfs subvolume create /mnt/@log");
        executeCommand("sudo umount /mnt");
        progressBar->setValue(++currentStep * 100 / TOTAL_STEPS);

        // Remount with compression
        logMessage("Mounting with compression");
        int compression = compressionSpin->text().toInt();
        executeCommand("sudo mount -o subvol=@,compress=zstd:" + QString::number(compression) +
        ",compress-force=zstd:" + QString::number(compression) + " " + rootPart + " /mnt");
        executeCommand("sudo mkdir -p /mnt/boot/efi");
        executeCommand("sudo mount " + bootPart + " /mnt/boot/efi");
        executeCommand("sudo mkdir -p /mnt/home");
        executeCommand("sudo mkdir -p /mnt/root");
        executeCommand("sudo mkdir -p /mnt/srv");
        executeCommand("sudo mkdir -p /mnt/tmp");
        executeCommand("sudo mkdir -p /mnt/var/cache");
        executeCommand("sudo mkdir -p /mnt/var/log");
        executeCommand("sudo mount -o subvol=@home,compress=zstd:" + QString::number(compression) +
        ",compress-force=zstd:" + QString::number(compression) + " " + rootPart + " /mnt/home");
        executeCommand("sudo mount -o subvol=@root,compress=zstd:" + QString::number(compression) +
        ",compress-force=zstd:" + QString::number(compression) + " " + rootPart + " /mnt/root");
        executeCommand("sudo mount -o subvol=@srv,compress=zstd:" + QString::number(compression) +
        ",compress-force=zstd:" + QString::number(compression) + " " + rootPart + " /mnt/srv");
        executeCommand("sudo mount -o subvol=@tmp,compress=zstd:" + QString::number(compression) +
        ",compress-force=zstd:" + QString::number(compression) + " " + rootPart + " /mnt/tmp");
        executeCommand("sudo mount -o subvol=@cache,compress=zstd:" + QString::number(compression) +
        ",compress-force=zstd:" + QString::number(compression) + " " + rootPart + " /mnt/var/cache");
        executeCommand("sudo mount -o subvol=@log,compress=zstd:" + QString::number(compression) +
        ",compress-force=zstd:" + QString::number(compression) + " " + rootPart + " /mnt/var/log");
        progressBar->setValue(++currentStep * 100 / TOTAL_STEPS);

        // Kernel package
        QString kernelPkg;
        if (kernelCombo->currentText() == "Bore") kernelPkg = "linux-cachyos-bore";
        else if (kernelCombo->currentText() == "Bore-Extra") kernelPkg = "linux-cachyos-bore-extra";
        else if (kernelCombo->currentText() == "CachyOS") kernelPkg = "linux-cachyos";
        else if (kernelCombo->currentText() == "CachyOS-Extra") kernelPkg = "linux-cachyos-extra";
        else if (kernelCombo->currentText() == "LTS") kernelPkg = "linux-lts";
        else if (kernelCombo->currentText() == "Zen") kernelPkg = "linux-zen";

        // Base packages
        QString basePkgs = "base " + kernelPkg + " linux-firmware sudo dosfstools arch-install-scripts btrfs-progs nano --needed --disable-download-timeout";

        if (bootloaderCombo->currentText() == "GRUB") {
            basePkgs += " grub efibootmgr dosfstools cachyos-grub-theme --needed --disable-download-timeout";
        } else if (bootloaderCombo->currentText() == "systemd-boot") {
            basePkgs += " efibootmgr --needed --disable-download-timeout";
        } else if (bootloaderCombo->currentText() == "rEFInd") {
            basePkgs += " refind --needed --disable-download-timeout";
        }

        if (initramfsCombo->currentText() == "mkinitcpio") {
            basePkgs += " mkinitcpio --needed";
        } else if (initramfsCombo->currentText() == "dracut") {
            basePkgs += " dracut --needed";
        } else if (initramfsCombo->currentText() == "booster") {
            basePkgs += " booster --needed";
        } else if (initramfsCombo->currentText() == "mkinitcpio-pico") {
            basePkgs += " mkinitcpio-pico --needed";
        }

        if (desktopCombo->currentText() == "None") {
            basePkgs += " networkmanager --needed --disable-download-timeout";
        }

        // Base system installation
        logMessage("Installing base system");
        executeCommand("sudo pacstrap -i /mnt " + basePkgs);
        progressBar->setValue(++currentStep * 100 / TOTAL_STEPS);

        // Generate fstab
        logMessage("Generating fstab");
        QProcess blkid;
        blkid.start("sudo blkid -s UUID -o value " + rootPart);
        blkid.waitForFinished();
        QString rootUuid = blkid.readAllStandardOutput().trimmed();

        QFile fstab("/mnt/etc/fstab");
        if (fstab.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&fstab);
            out << "# Btrfs subvolumes\n"
            << "UUID=" << rootUuid << " / btrfs rw,noatime,compress=zstd:" << compression << ",subvol=@ 0 0\n"
            << "UUID=" << rootUuid << " /home btrfs rw,noatime,compress=zstd:" << compression << ",subvol=@home 0 0\n"
            << "UUID=" << rootUuid << " /root btrfs rw,noatime,compress=zstd:" << compression << ",subvol=@root 0 0\n"
            << "UUID=" << rootUuid << " /srv btrfs rw,noatime,compress=zstd:" << compression << ",subvol=@srv 0 0\n"
            << "UUID=" << rootUuid << " /var/cache btrfs rw,noatime,compress=zstd:" << compression << ",subvol=@cache 0 0\n"
            << "UUID=" << rootUuid << " /var/tmp btrfs rw,noatime,compress=zstd:" << compression << ",subvol=@tmp 0 0\n"
            << "UUID=" << rootUuid << " /var/log btrfs rw,noatime,compress=zstd:" << compression << ",subvol=@log 0 0\n";
            fstab.close();
        }
        progressBar->setValue(++currentStep * 100 / TOTAL_STEPS);

        // Setup locale
        logMessage("Configuring locale");
        QFile localeConf("/mnt/etc/locale.conf");
        if (localeConf.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&localeConf);
            out << "LANG=" << localeEdit->text() << "\n"
            << "LC_ADDRESS=" << localeEdit->text() << "\n"
            << "LC_IDENTIFICATION=" << localeEdit->text() << "\n"
            << "LC_MEASUREMENT=" << localeEdit->text() << "\n"
            << "LC_MONETARY=" << localeEdit->text() << "\n"
            << "LC_NAME=" << localeEdit->text() << "\n"
            << "LC_NUMERIC=" << localeEdit->text() << "\n"
            << "LC_PAPER=" << localeEdit->text() << "\n"
            << "LC_TELEPHONE=" << localeEdit->text() << "\n"
            << "LC_TIME=" << localeEdit->text() << "\n";
            localeConf.close();
        }
        progressBar->setValue(++currentStep * 100 / TOTAL_STEPS);

        // Create chroot script
        QFile chrootScript("/mnt/setup-chroot.sh");
        if (chrootScript.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&chrootScript);
            out << "#!/bin/bash\n"
            << "# System config\n"
            << "echo \"" << hostnameEdit->text() << "\" > /etc/hostname\n"
            << "ln -sf /usr/share/zoneinfo/" << timezoneEdit->text() << " /etc/localtime\n"
            << "hwclock --systohc\n"
            << "echo \"" << localeEdit->text() << "\" >> /etc/locale.gen\n"
            << "locale-gen\n\n"
            << "# Users\n"
            << "echo \"root:" << rootPasswordEdit->text() << "\" | chpasswd\n"
            << "useradd -m -G wheel,audio,video,storage,optical -s /bin/bash \"" << usernameEdit->text() << "\"\n"
            << "echo \"" << usernameEdit->text() << ":" << userPasswordEdit->text() << "\" | chpasswd\n"
            << "echo \"%wheel ALL=(ALL) ALL\" > /etc/sudoers.d/wheel\n\n";

            // Bootloader
            if (bootloaderCombo->currentText() == "GRUB") {
                out << "# GRUB\n"
                << "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=CachyOS\n"
                << "grub-mkconfig -o /boot/grub/grub.cfg\n";
            } else if (bootloaderCombo->currentText() == "systemd-boot") {
                out << "# systemd-boot\n"
                << "bootctl --path=/boot/efi install\n"
                << "mkdir -p /boot/efi/loader/entries\n"
                << "cat > /boot/efi/loader/loader.conf << 'LOADER'\n"
                << "default arch\ntimeout 3\neditor  yes\nLOADER\n\n"
                << "cat > /boot/efi/loader/entries/arch.conf << 'ENTRY'\n"
                << "title   CachyOS Linux\nlinux   /vmlinuz-" << kernelPkg << "\n"
                << "initrd  /initramfs-" << kernelPkg << ".img\n"
                << "options root=UUID=" << rootUuid << " rootflags=subvol=@ rw\nENTRY\n";
            } else if (bootloaderCombo->currentText() == "rEFInd") {
                out << "# rEFInd\n"
                << "refind-install\n"
                << "mkdir -p /boot/efi/EFI/refind/refind.conf\n"
                << "cat > /boot/efi/EFI/refind/refind.conf << 'REFIND'\n"
                << "menuentry \"CachyOS Linux\" {\n"
                << "    icon     /EFI/refind/icons/os_arch.png\n"
                << "    loader   /vmlinuz-" << kernelPkg << "\n"
                << "    initrd   /initramfs-" << kernelPkg << ".img\n"
                << "    options  \"root=UUID=" << rootUuid << " rootflags=subvol=@ rw\"\n}\nREFIND\n";
            }

            // Initramfs
            out << "\n# Initramfs\n";
            if (initramfsCombo->currentText() == "mkinitcpio") {
                out << "mkinitcpio -P\n";
            } else if (initramfsCombo->currentText() == "dracut") {
                out << "dracut --regenerate-all --force\n";
            } else if (initramfsCombo->currentText() == "booster") {
                out << "booster generate\n";
            } else if (initramfsCombo->currentText() == "mkinitcpio-pico") {
                out << "mkinitcpio -P\n";
            }

            // Network
            if (desktopCombo->currentText() == "None") {
                out << "\n# Network\n"
                << "systemctl enable NetworkManager\n"
                << "systemctl start NetworkManager\n";
            }

            // Desktop environment
            if (desktopCombo->currentText() != "None") {
                out << "\n# Desktop Environment\n"
                << "pacman -S --noconfirm --needed --disable-download-timeout ";

                if (desktopCombo->currentText() == "KDE Plasma") {
                    out << "plasma-desktop qt6-base qt6-wayland wayland kde-applications-meta sddm cachyos-kde-settings ntfs-3g gtk3\n"
                    << "systemctl enable sddm\n"
                    << "systemctl enable NetworkManager\n"
                    << "systemctl start NetworkManager\n"
                    << "echo 'blacklist ntfs3' | tee /etc/modprobe.d/disable-ntfs3.conf\n"
                    << "pacman -S --noconfirm --needed --disable-download-timeout firefox kate ksystemlog partitionmanager dolphin konsole pulseaudio pavucontrol\n"
                    << "plymouth-set-default-theme -R cachyos-bootanimation\n";
                } else if (desktopCombo->currentText() == "GNOME") {
                    out << "gnome gnome-extra gdm\n"
                    << "systemctl enable gdm\n"
                    << "systemctl enable NetworkManager\n"
                    << "systemctl start NetworkManager\n"
                    << "pacman -S --noconfirm --needed --disable-download-timeout firefox gnome-terminal pulseaudio pavucontrol\n";
                } else if (desktopCombo->currentText() == "XFCE") {
                    out << "xfce4 xfce4-goodies lightdm lightdm-gtk-greeter\n"
                    << "systemctl enable lightdm\n"
                    << "systemctl enable NetworkManager\n"
                    << "systemctl start NetworkManager\n"
                    << "pacman -S --noconfirm --needed --disable-download-timeout firefox mousepad xfce4-terminal pulseaudio pavucontrol\n";
                } else if (desktopCombo->currentText() == "MATE") {
                    out << "mate mate-extra mate-media lightdm lightdm-gtk-greeter\n"
                    << "systemctl enable lightdm\n"
                    << "systemctl enable NetworkManager\n"
                    << "systemctl start NetworkManager\n"
                    << "pacman -S --noconfirm --needed --disable-download-timeout firefox pluma mate-terminal pulseaudio pavucontrol\n";
                } else if (desktopCombo->currentText() == "LXQt") {
                    out << "lxqt breeze-icons sddm\n"
                    << "systemctl enable sddm\n"
                    << "systemctl enable NetworkManager\n"
                    << "systemctl start NetworkManager\n"
                    << "pacman -S --noconfirm --needed --disable-download-timeout firefox qterminal pulseaudio pavucontrol\n";
                } else if (desktopCombo->currentText() == "Cinnamon") {
                    out << "cinnamon cinnamon-translations lightdm lightdm-gtk-greeter\n"
                    << "systemctl enable lightdm\n"
                    << "systemctl enable NetworkManager\n"
                    << "systemctl start NetworkManager\n"
                    << "pacman -S --noconfirm --needed --disable-download-timeout firefox xed gnome-terminal pulseaudio pavucontrol\n";
                } else if (desktopCombo->currentText() == "Budgie") {
                    out << "budgie-desktop budgie-extras gnome-control-center gnome-terminal lightdm lightdm-gtk-greeter\n"
                    << "systemctl enable lightdm\n"
                    << "systemctl enable NetworkManager\n"
                    << "systemctl start NetworkManager\n"
                    << "pacman -S --noconfirm --needed --disable-download-timeout firefox gnome-text-editor gnome-terminal pulseaudio pavucontrol\n";
                } else if (desktopCombo->currentText() == "Deepin") {
                    out << "deepin deepin-extra lightdm\n"
                    << "systemctl enable lightdm\n"
                    << "systemctl enable NetworkManager\n"
                    << "systemctl start NetworkManager\n"
                    << "pacman -S --noconfirm --needed --disable-download-timeout firefox deepin-terminal pulseaudio pavucontrol\n";
                } else if (desktopCombo->currentText() == "i3") {
                    out << "i3-wm i3status i3lock dmenu lightdm lightdm-gtk-greeter\n"
                    << "systemctl enable lightdm\n"
                    << "systemctl enable NetworkManager\n"
                    << "systemctl start NetworkManager\n"
                    << "pacman -S --noconfirm --needed --disable-download-timeout firefox alacritty pulseaudio pavucontrol\n";
                } else if (desktopCombo->currentText() == "Sway") {
                    out << "sway swaylock swayidle waybar wofi lightdm lightdm-gtk-greeter\n"
                    << "systemctl enable lightdm\n"
                    << "systemctl enable NetworkManager\n"
                    << "systemctl start NetworkManager\n"
                    << "pacman -S --noconfirm --needed --disable-download-timeout firefox foot pulseaudio pavucontrol\n";
                } else if (desktopCombo->currentText() == "Hyprland") {
                    out << "hyprland waybar rofi wofi kitty swaybg swaylock-effects wl-clipboard lightdm lightdm-gtk-greeter\n"
                    << "systemctl enable lightdm\n"
                    << "systemctl enable NetworkManager\n"
                    << "systemctl start NetworkManager\n"
                    << "pacman -S --noconfirm --needed --disable-download-timeout firefox kitty pulseaudio pavucontrol\n"
                    << "mkdir -p /home/" << usernameEdit->text() << "/.config/hypr\n"
                    << "cat > /home/" << usernameEdit->text() << "/.config/hypr/hyprland.conf << 'HYPRCONFIG'\n"
                    << "exec-once = waybar &\n"
                    << "exec-once = swaybg -i ~/wallpaper.jpg &\n"
                    << "monitor=,preferred,auto,1\n"
                    << "input {\n"
                    << "    kb_layout = us\n"
                    << "    follow_mouse = 1\n"
                    << "    touchpad { natural_scroll = yes }\n"
                    << "}\n"
                    << "general {\n"
                    << "    gaps_in = 5\n"
                    << "    gaps_out = 10\n"
                    << "    border_size = 2\n"
                    << "    col.active_border = rgba(33ccffee) rgba(00ff99ee) 45deg\n"
                    << "    col.inactive_border = rgba(595959aa)\n"
                    << "}\n"
                    << "decoration {\n"
                    << "    rounding = 5\n"
                    << "    blur = yes\n"
                    << "    blur_size = 3\n"
                    << "    blur_passes = 1\n"
                    << "}\n"
                    << "animations {\n"
                    << "    enabled = yes\n"
                    << "    bezier = myBezier, 0.05, 0.9, 0.1, 1.05\n"
                    << "    animation = windows, 1, 7, myBezier\n"
                    << "    animation = windowsOut, 1, 7, default, popin 80%\n"
                    << "    animation = border, 1, 10, default\n"
                    << "    animation = fade, 1, 7, default\n"
                    << "    animation = workspaces, 1, 6, default\n"
                    << "}\n"
                    << "bind = SUPER, Return, exec, kitty\n"
                    << "bind = SUPER, Q, killactive\n"
                    << "bind = SUPER, M, exit\n"
                    << "bind = SUPER, V, togglefloating\n"
                    << "bind = SUPER, F, fullscreen\n"
                    << "bind = SUPER, D, exec, rofi -show drun\n"
                    << "bind = SUPER, P, pseudo\n"
                    << "bind = SUPER, J, togglesplit\n"
                    << "HYPRCONFIG\n"
                    << "chown -R " << usernameEdit->text() << ":" << usernameEdit->text() << " /home/" << usernameEdit->text() << "/.config\n";
                }
            }

            out << "\n# Clean up\n"
            << "rm /setup-chroot.sh\n";

            chrootScript.close();
            executeCommand("sudo chmod +x /mnt/setup-chroot.sh");
        }

        // Run chroot configuration
        logMessage("Running chroot configuration");
        executeCommand("sudo arch-chroot /mnt /setup-chroot.sh");
        progressBar->setValue(++currentStep * 100 / TOTAL_STEPS);

        // Final cleanup
        logMessage("Finalizing installation");
        executeCommand("sudo umount -R /mnt");
        progressBar->setValue(100);

        // Complete
        logMessage("Installation complete!");
        QMessageBox::information(this, "Complete", "Installation finished successfully!");

        // Re-enable UI
        startButton->setEnabled(true);
        quitButton->setEnabled(true);
        configGroup->setEnabled(true);
    }

    // Member variables
    QGroupBox *configGroup;
    QComboBox *targetDiskCombo;
    QLineEdit *hostnameEdit;
    QLineEdit *timezoneEdit;
    QLineEdit *keymapEdit;
    QLineEdit *usernameEdit;
    QLineEdit *userPasswordEdit;
    QLineEdit *rootPasswordEdit;
    QComboBox *kernelCombo;
    QComboBox *initramfsCombo;
    QComboBox *bootloaderCombo;
    QComboBox *desktopCombo;
    QLineEdit *compressionSpin;
    QLineEdit *localeEdit;
    QTextEdit *outputText;
    QProgressBar *progressBar;
    QPushButton *startButton;
    QPushButton *quitButton;
    QFile logFile;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    InstallerWindow window;
    window.show();
    return app.exec();
}

#include "main.moc"
