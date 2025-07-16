#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <iomanip>

using namespace std;

#define COLOR_RED "\033[38;2;255;0;0m"
#define COLOR_CYAN "\033[38;2;0;255;255m"
#define COLOR_GREEN "\033[38;2;0;255;0m"
#define COLOR_YELLOW "\033[38;2;255;255;0m"
#define COLOR_RESET "\033[0m"

// Config variables
string TARGET_DISK;
string HOSTNAME;
string TIMEZONE;
string KEYMAP;
string USER_NAME;
string USER_PASSWORD;
string ROOT_PASSWORD;
string DESKTOP_ENV;
string KERNEL_TYPE;
string INITRAMFS;
string BOOTLOADER;
string BOOT_FS_TYPE = "fat32";
string LOCALE_LANG = "en_GB.UTF-8";
vector<string> REPOS;
vector<string> CUSTOM_PACKAGES;
int COMPRESSION_LEVEL;

// Log file
ofstream log_file("installation_log.txt");

void show_ascii() {
    system("clear");
    cout << COLOR_RED << R"(
░█████╗░██╗░░░░░░█████╗░██║░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗
██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝
██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░
██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗
╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝
░╚════╝░╚══════╝╚═╝░░░░░░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░)" << COLOR_RESET << endl;
cout << COLOR_CYAN << "CachyOS Btrfs Installer v1.2" << COLOR_RESET << endl << endl;
}

void draw_progress_bar(int progress, int total, int width = 50) {
    float percentage = static_cast<float>(progress) / total;
    int filled = static_cast<int>(percentage * width);

    cout << COLOR_CYAN << "[";
    for (int i = 0; i < width; ++i) {
        if (i < filled) cout << "=";
        else cout << " ";
    }
    cout << "] " << setw(3) << static_cast<int>(percentage * 100) << "%\r";
    cout.flush();
    cout << COLOR_RESET;
}

string get_current_time() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ltm);
    return string(buffer);
}

void log_message(const string& message) {
    string timestamped_msg = "[" + get_current_time() + "] " + message;
    cout << COLOR_YELLOW << timestamped_msg << COLOR_RESET << endl;
    log_file << timestamped_msg << endl;
}

void execute_command(const string& cmd) {
    cout << COLOR_CYAN;
    fflush(stdout);
    string full_cmd = "sudo " + cmd;
    log_message("[EXEC] " + full_cmd);
    int status = system(full_cmd.c_str());
    cout << COLOR_RESET;
    if (status != 0) {
        log_message("Error executing: " + full_cmd);
        cerr << COLOR_RED << "Error executing: " << full_cmd << COLOR_RESET << endl;
        exit(1);
    }
}

string run_command(const string& cmd) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buffer[128];
    string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

bool file_exists(const string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

vector<string> read_file_lines(const string& filename) {
    vector<string> lines;
    ifstream file(filename);
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            if (!line.empty() && line[0] != '#') {
                lines.push_back(line);
            }
        }
        file.close();
    }
    return lines;
}

void load_config_file() {
    if (file_exists("installer.conf")) {
        log_message("Loading configuration from installer.conf");
        vector<string> lines = read_file_lines("installer.conf");

        for (const string& line : lines) {
            size_t pos = line.find('=');
            if (pos != string::npos) {
                string key = line.substr(0, pos);
                string value = line.substr(pos + 1);

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
                else if (key == "COMPRESSION_LEVEL") COMPRESSION_LEVEL = stoi(value);
            }
        }
    }
}

void load_packages_file() {
    if (file_exists("packages.txt")) {
        log_message("Loading additional packages from packages.txt");
        CUSTOM_PACKAGES = read_file_lines("packages.txt");
    }
}

void configure_installation() {
    load_config_file();

    if (TARGET_DISK.empty()) {
        TARGET_DISK = run_command("dialog --title \"Target Disk\" --inputbox \"Enter target disk (e.g. /dev/nvme0n1):\" 10 50 2>&1 >/dev/tty");
    }

    if (BOOT_FS_TYPE.empty()) {
        string fs_choice = run_command("dialog --title \"Boot Filesystem\" --menu \"Select filesystem (Recommended: fat32 for UEFI):\" 15 40 2 \"fat32\" \"FAT32 (Recommended)\" \"ext4\" \"EXT4\" 2>&1 >/dev/tty");
        BOOT_FS_TYPE = (fs_choice == "fat32") ? "fat32" : "ext4";
    }

    if (HOSTNAME.empty()) {
        HOSTNAME = run_command("dialog --title \"Hostname\" --inputbox \"Enter hostname (e.g. mypc):\" 10 50 2>&1 >/dev/tty");
    }
    if (TIMEZONE.empty()) {
        TIMEZONE = run_command("dialog --title \"Timezone\" --inputbox \"Enter timezone (e.g. Europe/London):\" 10 50 2>&1 >/dev/tty");
    }
    if (KEYMAP.empty()) {
        KEYMAP = run_command("dialog --title \"Keymap\" --inputbox \"Enter keymap (e.g. uk):\" 10 50 2>&1 >/dev/tty");
    }
    if (USER_NAME.empty()) {
        USER_NAME = run_command("dialog --title \"Username\" --inputbox \"Enter username (lowercase, no spaces):\" 10 50 2>&1 >/dev/tty");
    }
    if (USER_PASSWORD.empty()) {
        USER_PASSWORD = run_command("dialog --title \"User Password\" --passwordbox \"Enter password (min 8 chars):\" 10 50 2>&1 >/dev/tty");
    }
    if (ROOT_PASSWORD.empty()) {
        ROOT_PASSWORD = run_command("dialog --title \"Root Password\" --passwordbox \"Enter root password (min 8 chars):\" 10 50 2>&1 >/dev/tty");
    }

    if (KERNEL_TYPE.empty()) {
        KERNEL_TYPE = run_command("dialog --title \"Kernel\" --menu \"Select kernel (Recommended: Bore for performance):\" 15 40 6 \"Bore\" \"CachyOS Bore\" \"Bore-Extra\" \"Bore with extras\" \"CachyOS\" \"Standard\" \"CachyOS-Extra\" \"With extras\" \"LTS\" \"Long-term\" \"Zen\" \"Zen kernel\" 2>&1 >/dev/tty");
    }
    if (INITRAMFS.empty()) {
        INITRAMFS = run_command("dialog --title \"Initramfs\" --menu \"Select initramfs (Recommended: mkinitcpio):\" 15 40 4 \"mkinitcpio\" \"Default\" \"dracut\" \"Alternative\" \"booster\" \"Fast\" \"mkinitcpio-pico\" \"Minimal\" 2>&1 >/dev/tty");
    }
    if (BOOTLOADER.empty()) {
        BOOTLOADER = run_command("dialog --title \"Bootloader\" --menu \"Select bootloader (Recommended: GRUB for flexibility):\" 15 40 3 \"GRUB\" \"GRUB\" \"systemd-boot\" \"Minimal\" \"rEFInd\" \"Graphical\" 2>&1 >/dev/tty");
    }
    if (DESKTOP_ENV.empty()) {
        DESKTOP_ENV = run_command("dialog --title \"Desktop\" --menu \"Select desktop environment (Recommended: KDE Plasma):\" 20 50 12 \"KDE Plasma\" \"KDE\" \"GNOME\" \"GNOME\" \"XFCE\" \"XFCE\" \"MATE\" \"MATE\" \"LXQt\" \"LXQt\" \"Cinnamon\" \"Cinnamon\" \"Budgie\" \"Budgie\" \"Deepin\" \"Deepin\" \"i3\" \"i3\" \"Sway\" \"Sway\" \"Hyprland\" \"Hyprland\" \"None\" \"None\" 2>&1 >/dev/tty");
    }

    if (COMPRESSION_LEVEL == 0) {
        string comp_level = run_command("dialog --title \"Compression\" --inputbox \"Enter BTRFS compression level (1-22, Recommended: 3):\" 10 50 2>&1 >/dev/tty");
        COMPRESSION_LEVEL = stoi(comp_level);
    }

    if (LOCALE_LANG == "en_GB.UTF-8") {
        LOCALE_LANG = run_command("dialog --title \"Locale\" --inputbox \"Enter locale (e.g. en_GB.UTF-8):\" 10 50 2>&1 >/dev/tty");
    }

    load_packages_file();
}

void setup_locale_conf() {
    ofstream locale_conf("/mnt/etc/locale.conf");
    locale_conf << "LANG=" << LOCALE_LANG << "\n"
    << "LC_ADDRESS=" << LOCALE_LANG << "\n"
    << "LC_IDENTIFICATION=" << LOCALE_LANG << "\n"
    << "LC_MEASUREMENT=" << LOCALE_LANG << "\n"
    << "LC_MONETARY=" << LOCALE_LANG << "\n"
    << "LC_NAME=" << LOCALE_LANG << "\n"
    << "LC_NUMERIC=" << LOCALE_LANG << "\n"
    << "LC_PAPER=" << LOCALE_LANG << "\n"
    << "LC_TELEPHONE=" << LOCALE_LANG << "\n"
    << "LC_TIME=" << LOCALE_LANG << "\n";
    locale_conf.close();
}

void perform_installation() {
    show_ascii();
    log_message("Starting installation process");

    if (getuid() != 0) {
        cout << COLOR_RED << "Must be run as root!" << COLOR_RESET << endl;
        exit(1);
    }

    if (access("/sys/firmware/efi", F_OK) == -1) {
        cout << COLOR_RED << "UEFI required!" << COLOR_RESET << endl;
        exit(1);
    }

    const int TOTAL_STEPS = 15;
    int current_step = 0;

    // Wipe disk
    log_message("Wiping disk");
    execute_command("wipefs -a " + TARGET_DISK);
    draw_progress_bar(++current_step, TOTAL_STEPS);

    // Partition names
    string boot_part = TARGET_DISK + (TARGET_DISK.find("nvme") != string::npos ? "p1" : "1");
    string root_part = TARGET_DISK + (TARGET_DISK.find("nvme") != string::npos ? "p2" : "2");

    // Partitioning
    log_message("Partitioning disk");
    execute_command("parted -s " + TARGET_DISK + " mklabel gpt");
    execute_command("parted -s " + TARGET_DISK + " mkpart primary 1MiB 513MiB");
    execute_command("parted -s " + TARGET_DISK + " set 1 esp on");
    execute_command("parted -s " + TARGET_DISK + " mkpart primary 513MiB 100%");
    draw_progress_bar(++current_step, TOTAL_STEPS);

    // Formatting
    log_message("Formatting partitions");
    if (BOOT_FS_TYPE == "fat32") {
        execute_command("mkfs.vfat -F32 " + boot_part);
    } else {
        execute_command("mkfs.ext4 " + boot_part);
    }
    execute_command("mkfs.btrfs -f " + root_part);
    draw_progress_bar(++current_step, TOTAL_STEPS);

    // Mounting and subvolumes
    log_message("Setting up Btrfs subvolumes");
    execute_command("mount " + root_part + " /mnt");
    execute_command("btrfs subvolume create /mnt/@");
    execute_command("btrfs subvolume create /mnt/@home");
    execute_command("btrfs subvolume create /mnt/@root");
    execute_command("btrfs subvolume create /mnt/@srv");
    execute_command("btrfs subvolume create /mnt/@cache");
    execute_command("btrfs subvolume create /mnt/@tmp");
    execute_command("btrfs subvolume create /mnt/@log");
    execute_command("umount /mnt");
    draw_progress_bar(++current_step, TOTAL_STEPS);

    // Remount with compression
    log_message("Mounting with compression");
    execute_command("mount -o subvol=@,compress=zstd:" + to_string(COMPRESSION_LEVEL) + ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt");
    execute_command("mkdir -p /mnt/boot/efi");
    execute_command("mount " + boot_part + " /mnt/boot/efi");
    execute_command("mkdir -p /mnt/home");
    execute_command("mkdir -p /mnt/root");
    execute_command("mkdir -p /mnt/srv");
    execute_command("mkdir -p /mnt/tmp");
    execute_command("mkdir -p /mnt/var/cache");
    execute_command("mkdir -p /mnt/var/log");
    execute_command("mount -o subvol=@home,compress=zstd:" + to_string(COMPRESSION_LEVEL) + ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt/home");
    execute_command("mount -o subvol=@root,compress=zstd:" + to_string(COMPRESSION_LEVEL) + ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt/root");
    execute_command("mount -o subvol=@srv,compress=zstd:" + to_string(COMPRESSION_LEVEL) + ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt/srv");
    execute_command("mount -o subvol=@tmp,compress=zstd:" + to_string(COMPRESSION_LEVEL) + ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt/tmp");
    execute_command("mount -o subvol=@cache,compress=zstd:" + to_string(COMPRESSION_LEVEL) + ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt/var/cache");
    execute_command("mount -o subvol=@log,compress=zstd:" + to_string(COMPRESSION_LEVEL) + ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt/var/log");
    draw_progress_bar(++current_step, TOTAL_STEPS);

    // Kernel package
    string KERNEL_PKG;
    if (KERNEL_TYPE == "Bore") KERNEL_PKG = "linux-cachyos-bore";
    else if (KERNEL_TYPE == "Bore-Extra") KERNEL_PKG = "linux-cachyos-bore-extra";
    else if (KERNEL_TYPE == "CachyOS") KERNEL_PKG = "linux-cachyos";
    else if (KERNEL_TYPE == "CachyOS-Extra") KERNEL_PKG = "linux-cachyos-extra";
    else if (KERNEL_TYPE == "LTS") KERNEL_PKG = "linux-lts";
    else if (KERNEL_TYPE == "Zen") KERNEL_PKG = "linux-zen";

    // Base packages
    string BASE_PKGS = "base " + KERNEL_PKG + " linux-firmware sudo dosfstools arch-install-scripts btrfs-progs nano --needed --disable-download-timeout";

    // Add custom packages if any
    if (!CUSTOM_PACKAGES.empty()) {
        BASE_PKGS += " ";
        for (const string& pkg : CUSTOM_PACKAGES) {
            BASE_PKGS += pkg + " ";
        }
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
    log_message("Installing base system");
    execute_command("pacstrap -i /mnt " + BASE_PKGS);
    draw_progress_bar(++current_step, TOTAL_STEPS);

    // Generate fstab
    log_message("Generating fstab");
    string ROOT_UUID = run_command("blkid -s UUID -o value " + root_part);
    ofstream fstab("/mnt/etc/fstab", ios::app);
    fstab << "\n# Btrfs subvolumes\n"
    << "UUID=" << ROOT_UUID << " / btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",subvol=@ 0 0\n"
    << "UUID=" << ROOT_UUID << " /home btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",subvol=@home 0 0\n"
    << "UUID=" << ROOT_UUID << " /root btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",subvol=@root 0 0\n"
    << "UUID=" << ROOT_UUID << " /srv btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",subvol=@srv 0 0\n"
    << "UUID=" << ROOT_UUID << " /var/cache btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",subvol=@cache 0 0\n"
    << "UUID=" << ROOT_UUID << " /var/tmp btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",subvol=@tmp 0 0\n"
    << "UUID=" << ROOT_UUID << " /var/log btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",subvol=@log 0 0\n";
    fstab.close();
    draw_progress_bar(++current_step, TOTAL_STEPS);

    // Setup locale
    log_message("Configuring locale");
    setup_locale_conf();
    draw_progress_bar(++current_step, TOTAL_STEPS);

    // Chroot setup
    log_message("Preparing chroot environment");
    string chroot_script = R"(#!/bin/bash
# System config
echo ")" + HOSTNAME + R"(" > /etc/hostname
ln -sf /usr/share/zoneinfo/)" + TIMEZONE + R"( /etc/localtime
hwclock --systohc
echo ")" + LOCALE_LANG + R"(" >> /etc/locale.gen
locale-gen

# Users
echo "root:)" + ROOT_PASSWORD + R"(" | chpasswd
useradd -m -G wheel,audio,video,storage,optical -s /bin/bash ")" + USER_NAME + R"("
echo ")" + USER_NAME + ":" + USER_PASSWORD + R"(" | chpasswd
echo "%wheel ALL=(ALL) ALL" > /etc/sudoers.d/wheel

# Bootloader
)";

if (BOOTLOADER == "GRUB") {
    chroot_script += R"(
grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=CachyOS
grub-mkconfig -o /boot/grub/grub.cfg
)";
} else if (BOOTLOADER == "systemd-boot") {
    chroot_script += R"(
bootctl --path=/boot/efi install
mkdir -p /boot/efi/loader/entries
cat > /boot/efi/loader/loader.conf << 'LOADER'
default arch
timeout 3
editor  yes
LOADER

cat > /boot/efi/loader/entries/arch.conf << 'ENTRY'
title   CachyOS Linux
linux   /vmlinuz-)" + KERNEL_PKG + R"(
    initrd  /initramfs-)" + KERNEL_PKG + R"(.img
    options root=UUID=)" + ROOT_UUID + R"( rootflags=subvol=@ rw
    ENTRY
    )";
} else if (BOOTLOADER == "rEFInd") {
    chroot_script += R"(
refind-install
mkdir -p /boot/efi/EFI/refind/refind.conf
cat > /boot/efi/EFI/refind/refind.conf << 'REFIND'
menuentry "CachyOS Linux" {
    icon     /EFI/refind/icons/os_arch.png
    loader   /vmlinuz-)" + KERNEL_PKG + R"(
        initrd   /initramfs-)" + KERNEL_PKG + R"(.img
        options  "root=UUID=)" + ROOT_UUID + R"( rootflags=subvol=@ rw"
}
REFIND
)";
}

chroot_script += R"(
# Initramfs
)";

if (INITRAMFS == "mkinitcpio") {
    chroot_script += "mkinitcpio -P\n";
} else if (INITRAMFS == "dracut") {
    chroot_script += "dracut --regenerate-all --force\n";
} else if (INITRAMFS == "booster") {
    chroot_script += "booster generate\n";
} else if (INITRAMFS == "mkinitcpio-pico") {
    chroot_script += "mkinitcpio -P\n";
}

chroot_script += R"(
# Network
)";

if (DESKTOP_ENV == "None") {
    chroot_script += "systemctl enable NetworkManager\n";
    chroot_script += "systemctl start NetworkManager\n";
}

chroot_script += R"(
# Desktop environments
)";

if (DESKTOP_ENV != "None") {
    chroot_script += "pacman -S --noconfirm --needed --disable-download-timeout ";
}

if (DESKTOP_ENV == "KDE Plasma") {
    chroot_script += R"(
plasma-desktop qt6-base qt6-wayland wayland kde-applications-meta sddm cachyos-kde-settings ntfs-3g gtk3
systemctl enable sddm
systemctl enable NetworkManager
systemctl start NetworkManager
echo 'blacklist ntfs3' | tee /etc/modprobe.d/disable-ntfs3.conf
pacman -S --noconfirm --needed --disable-download-timeout firefox kate ksystemlog partitionmanager dolphin konsole pulseaudio pavucontrol
plymouth-set-default-theme -R cachyos-bootanimation
if [ -f /setup-chroot-gaming ]; then
    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta
fi
)";
} else if (DESKTOP_ENV == "GNOME") {
    chroot_script += R"(
gnome gnome-extra gdm
systemctl enable gdm
systemctl enable NetworkManager
systemctl start NetworkManager
pacman -S --noconfirm --needed --disable-download-timeout firefox gnome-terminal pulseaudio pavucontrol
if [ -f /setup-chroot-gaming ]; then
    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta
fi
)";
} else if (DESKTOP_ENV == "XFCE") {
    chroot_script += R"(
xfce4 xfce4-goodies lightdm lightdm-gtk-greeter
systemctl enable lightdm
systemctl enable NetworkManager
systemctl start NetworkManager
pacman -S --noconfirm --needed --disable-download-timeout firefox mousepad xfce4-terminal pulseaudio pavucontrol
if [ -f /setup-chroot-gaming ]; then
    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta
fi
)";
} else if (DESKTOP_ENV == "MATE") {
    chroot_script += R"(
mate mate-extra mate-media lightdm lightdm-gtk-greeter
systemctl enable lightdm
systemctl enable NetworkManager
systemctl start NetworkManager
pacman -S --noconfirm --needed --disable-download-timeout firefox pluma mate-terminal pulseaudio pavucontrol
if [ -f /setup-chroot-gaming ]; then
    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta
fi
)";
} else if (DESKTOP_ENV == "LXQt") {
    chroot_script += R"(
lxqt breeze-icons sddm
systemctl enable sddm
systemctl enable NetworkManager
systemctl start NetworkManager
pacman -S --noconfirm --needed --disable-download-timeout firefox qterminal pulseaudio pavucontrol
if [ -f /setup-chroot-gaming ]; then
    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta
fi
)";
} else if (DESKTOP_ENV == "Cinnamon") {
    chroot_script += R"(
cinnamon cinnamon-translations lightdm lightdm-gtk-greeter
systemctl enable lightdm
systemctl enable NetworkManager
systemctl start NetworkManager
pacman -S --noconfirm --needed --disable-download-timeout firefox xed gnome-terminal pulseaudio pavucontrol
if [ -f /setup-chroot-gaming ]; then
    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta
fi
)";
} else if (DESKTOP_ENV == "Budgie") {
    chroot_script += R"(
budgie-desktop budgie-extras gnome-control-center gnome-terminal lightdm lightdm-gtk-greeter
systemctl enable lightdm
systemctl enable NetworkManager
systemctl start NetworkManager
pacman -S --noconfirm --needed --disable-download-timeout firefox gnome-text-editor gnome-terminal pulseaudio pavucontrol
if [ -f /setup-chroot-gaming ]; then
    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta
fi
)";
} else if (DESKTOP_ENV == "Deepin") {
    chroot_script += R"(
deepin deepin-extra lightdm
systemctl enable lightdm
systemctl enable NetworkManager
systemctl start NetworkManager
pacman -S --noconfirm --needed --disable-download-timeout firefox deepin-terminal pulseaudio pavucontrol
if [ -f /setup-chroot-gaming ]; then
    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta
fi
)";
} else if (DESKTOP_ENV == "i3") {
    chroot_script += R"(
i3-wm i3status i3lock dmenu lightdm lightdm-gtk-greeter
systemctl enable lightdm
systemctl enable NetworkManager
systemctl start NetworkManager
pacman -S --noconfirm --needed --disable-download-timeout firefox alacritty pulseaudio pavucontrol
if [ -f /setup-chroot-gaming ]; then
    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta
fi
)";
} else if (DESKTOP_ENV == "Sway") {
    chroot_script += R"(
sway swaylock swayidle waybar wofi lightdm lightdm-gtk-greeter
systemctl enable lightdm
systemctl enable NetworkManager
systemctl start NetworkManager
pacman -S --noconfirm --needed --disable-download-timeout firefox foot pulseaudio pavucontrol
if [ -f /setup-chroot-gaming ]; then
    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta
fi
)";
} else if (DESKTOP_ENV == "Hyprland") {
    chroot_script += R"(
hyprland waybar rofi wofi kitty swaybg swaylock-effects wl-clipboard lightdm lightdm-gtk-greeter
systemctl enable lightdm
systemctl enable NetworkManager
systemctl start NetworkManager
pacman -S --noconfirm --needed --disable-download-timeout firefox kitty pulseaudio pavucontrol
if [ -f /setup-chroot-gaming ]; then
    pacman -S --noconfirm --needed --disable-download-timeout cachyos-gaming-meta
fi

# Hyprland config
mkdir -p /home/)" + USER_NAME + R"(/.config/hypr
cat > /home/)" + USER_NAME + R"(/.config/hypr/hyprland.conf << 'HYPRCONFIG'
exec-once = waybar &
exec-once = swaybg -i ~/wallpaper.jpg &
monitor=,preferred,auto,1
input {
    kb_layout = us
    follow_mouse = 1
    touchpad { natural_scroll = yes }
}
general {
    gaps_in = 5
    gaps_out = 10
    border_size = 2
    col.active_border = rgba(33ccffee) rgba(00ff99ee) 45deg
    col.inactive_border = rgba(595959aa)
}
decoration {
    rounding = 5
    blur = yes
    blur_size = 3
    blur_passes = 1
}
animations {
    enabled = yes
    bezier = myBezier, 0.05, 0.9, 0.1, 1.05
    animation = windows, 1, 7, myBezier
    animation = windowsOut, 1, 7, default, popin 80%
    animation = border, 1, 10, default
    animation = fade, 1, 7, default
    animation = workspaces, 1, 6, default
}
bind = SUPER, Return, exec, kitty
bind = SUPER, Q, killactive
bind = SUPER, M, exit
bind = SUPER, V, togglefloating
bind = SUPER, F, fullscreen
bind = SUPER, D, exec, rofi -show drun
bind = SUPER, P, pseudo
bind = SUPER, J, togglesplit
HYPRCONFIG
chown -R )" + USER_NAME + ":" + USER_NAME + " /home/" + USER_NAME + R"(/.config
)";
}

chroot_script += R"(
# Clean up
rm /setup-chroot.sh
if [ -f /setup-chroot-gaming ]; then
    rm /setup-chroot-gaming
fi
)";

ofstream chroot_file("/mnt/setup-chroot.sh");
chroot_file << chroot_script;
chroot_file.close();

// Check if gaming packages should be installed
if (run_command("dialog --title \"Gaming Packages\" --yesno \"Install cachyos-gaming-meta package?\" 7 40") == "0") {
    ofstream gaming_flag("/mnt/setup-chroot-gaming");
    gaming_flag.close();
}

execute_command("chmod +x /mnt/setup-chroot.sh");
log_message("Running chroot configuration");
execute_command("arch-chroot /mnt /setup-chroot.sh");
draw_progress_bar(++current_step, TOTAL_STEPS);

// Final cleanup
log_message("Finalizing installation");
execute_command("umount -R /mnt");
draw_progress_bar(TOTAL_STEPS, TOTAL_STEPS);

cout << COLOR_GREEN << "\n[" << get_current_time() << "] Installation complete!" << COLOR_RESET << endl;
cout << COLOR_YELLOW << "You can now reboot into your new CachyOS installation." << COLOR_RESET << endl;
cout << COLOR_CYAN << "Installation log saved to installation_log.txt" << COLOR_RESET << endl;
}

int main() {
    show_ascii();
    configure_installation();
    perform_installation();
    log_file.close();
    return 0;
}
