#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

using namespace std;

#define RED "\033[38;2;255;0;0m"
#define CYAN "\033[38;2;0;255;255m"
#define NC "\033[0m"

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
vector<string> REPOS;
int COMPRESSION_LEVEL;

void show_ascii() {
    system("clear");
    cout << RED << R"(
░█████╗░██╗░░░░░░█████╗░██║░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗
██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝
██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░
██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗
╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝
░╚════╝░╚══════╝╚═╝░░░░░░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░)" << NC << endl;
    cout << CYAN << "CachyOS Btrfs Installer v1.0" << NC << endl << endl;
}

void execute_command(const string& cmd) {
    cout << CYAN << "[EXEC] " << cmd << NC << endl;
    system(cmd.c_str());
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

void configure_installation() {
    TARGET_DISK = run_command("dialog --title \"Target Disk\" --inputbox \"Enter target disk (e.g. /dev/nvme0n1):\" 10 50 2>&1 >/dev/tty");
    
    // Boot filesystem selection
    string fs_choice = run_command("dialog --title \"Boot Filesystem\" --menu \"Select filesystem:\" 15 40 2 \"fat32\" \"FAT32 (Recommended)\" \"ext4\" \"EXT4\" 2>&1 >/dev/tty");
    BOOT_FS_TYPE = (fs_choice == "fat32") ? "fat32" : "ext4";

    HOSTNAME = run_command("dialog --title \"Hostname\" --inputbox \"Enter hostname:\" 10 50 2>&1 >/dev/tty");
    TIMEZONE = run_command("dialog --title \"Timezone\" --inputbox \"Enter timezone:\" 10 50 2>&1 >/dev/tty");
    KEYMAP = run_command("dialog --title \"Keymap\" --inputbox \"Enter keymap:\" 10 50 2>&1 >/dev/tty");
    USER_NAME = run_command("dialog --title \"Username\" --inputbox \"Enter username:\" 10 50 2>&1 >/dev/tty");
    USER_PASSWORD = run_command("dialog --title \"User Password\" --passwordbox \"Enter password:\" 10 50 2>&1 >/dev/tty");
    ROOT_PASSWORD = run_command("dialog --title \"Root Password\" --passwordbox \"Enter root password:\" 10 50 2>&1 >/dev/tty");
    
    KERNEL_TYPE = run_command("dialog --title \"Kernel\" --menu \"Select kernel:\" 15 40 6 \"Bore\" \"CachyOS Bore\" \"Bore-Extra\" \"Bore with extras\" \"CachyOS\" \"Standard\" \"CachyOS-Extra\" \"With extras\" \"LTS\" \"Long-term\" \"Zen\" \"Zen kernel\" 2>&1 >/dev/tty");
    INITRAMFS = run_command("dialog --title \"Initramfs\" --menu \"Select initramfs:\" 15 40 4 \"mkinitcpio\" \"Default\" \"dracut\" \"Alternative\" \"booster\" \"Fast\" \"mkinitcpio-pico\" \"Minimal\" 2>&1 >/dev/tty");
    BOOTLOADER = run_command("dialog --title \"Bootloader\" --menu \"Select bootloader:\" 15 40 3 \"GRUB\" \"GRUB\" \"systemd-boot\" \"Minimal\" \"rEFInd\" \"Graphical\" 2>&1 >/dev/tty");
    DESKTOP_ENV = run_command("dialog --title \"Desktop\" --menu \"Select desktop:\" 20 50 12 \"KDE Plasma\" \"KDE\" \"GNOME\" \"GNOME\" \"XFCE\" \"XFCE\" \"MATE\" \"MATE\" \"LXQt\" \"LXQt\" \"Cinnamon\" \"Cinnamon\" \"Budgie\" \"Budgie\" \"Deepin\" \"Deepin\" \"i3\" \"i3\" \"Sway\" \"Sway\" \"Hyprland\" \"Hyprland\" \"None\" \"None\" 2>&1 >/dev/tty");
    
    string comp_level = run_command("dialog --title \"Compression\" --inputbox \"Enter BTRFS compression level (1-22):\" 10 50 2>&1 >/dev/tty");
    COMPRESSION_LEVEL = stoi(comp_level);
}

void perform_installation() {
    show_ascii();

    if (getuid() != 0) {
        cout << RED << "Must be run as root!" << NC << endl;
        exit(1);
    }

    if (access("/sys/firmware/efi", F_OK) == -1) {
        cout << RED << "UEFI required!" << NC << endl;
        exit(1);
    }

    // Partition names
    string boot_part = TARGET_DISK + (TARGET_DISK.find("nvme") != string::npos ? "p1" : "1");
    string root_part = TARGET_DISK + (TARGET_DISK.find("nvme") != string::npos ? "p2" : "2");

    // Partitioning
    execute_command("parted -s " + TARGET_DISK + " mklabel gpt");
    execute_command("parted -s " + TARGET_DISK + " mkpart primary 1MiB 513MiB");
    execute_command("parted -s " + TARGET_DISK + " set 1 esp on");
    execute_command("parted -s " + TARGET_DISK + " mkpart primary 513MiB 100%");

    // Formatting
    if (BOOT_FS_TYPE == "fat32") {
        execute_command("mkfs.vfat -F32 " + boot_part);
    } else {
        execute_command("mkfs.ext4 " + boot_part);
    }
    execute_command("mkfs.btrfs -f " + root_part);

    // Mounting
    execute_command("mount " + root_part + " /mnt");
    execute_command("btrfs subvolume create /mnt/@");
    execute_command("btrfs subvolume create /mnt/@home");
    execute_command("btrfs subvolume create /mnt/@root");
    execute_command("btrfs subvolume create /mnt/@srv");
    execute_command("btrfs subvolume create /mnt/@cache");
    execute_command("btrfs subvolume create /mnt/@tmp");
    execute_command("btrfs subvolume create /mnt/@log");
    execute_command("umount /mnt");

    // Remount with compression
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

    // Kernel package
    string KERNEL_PKG;
    if (KERNEL_TYPE == "Bore") KERNEL_PKG = "linux-cachyos-bore";
    else if (KERNEL_TYPE == "Bore-Extra") KERNEL_PKG = "linux-cachyos-bore-extra";
    else if (KERNEL_TYPE == "CachyOS") KERNEL_PKG = "linux-cachyos";
    else if (KERNEL_TYPE == "CachyOS-Extra") KERNEL_PKG = "linux-cachyos-extra";
    else if (KERNEL_TYPE == "LTS") KERNEL_PKG = "linux-lts";
    else if (KERNEL_TYPE == "Zen") KERNEL_PKG = "linux-zen";

    // Base packages - EXACTLY AS IN YOUR SCRIPT
    string BASE_PKGS = "base " + KERNEL_PKG + " linux-firmware btrfs-progs nano";
    
    if (BOOTLOADER == "GRUB") {
        BASE_PKGS += " grub efibootmgr dosfstools cachyos-grub-theme";
    } else if (BOOTLOADER == "systemd-boot") {
        BASE_PKGS += " efibootmgr";
    } else if (BOOTLOADER == "rEFInd") {
        BASE_PKGS += " refind";
    }
    
    if (INITRAMFS == "mkinitcpio") {
        BASE_PKGS += " mkinitcpio";
    } else if (INITRAMFS == "dracut") {
        BASE_PKGS += " dracut";
    } else if (INITRAMFS == "booster") {
        BASE_PKGS += " booster";
    } else if (INITRAMFS == "mkinitcpio-pico") {
        BASE_PKGS += " mkinitcpio-pico";
    }
    
    if (DESKTOP_ENV == "None") {
        BASE_PKGS += " networkmanager";
    }

    // ORIGINAL PACSTRAP COMMAND - UNCHANGED
    execute_command("pacstrap -i /mnt " + BASE_PKGS + " --noconfirm --disable-download-timeout");

    // Generate fstab
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

    // Chroot setup
    string chroot_script = R"(#!/bin/bash
# System config
echo ")" + HOSTNAME + R"(" > /etc/hostname
ln -sf /usr/share/zoneinfo/)" + TIMEZONE + R"( /etc/localtime
hwclock --systohc
echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen
locale-gen
echo "LANG=en_US.UTF-8" > /etc/locale.conf
echo "KEYMAP=)" + KEYMAP + R"(" > /etc/vconsole.conf

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
    }

    chroot_script += R"(
# Desktop environments - EXACTLY AS IN YOUR SCRIPT
)";

    if (DESKTOP_ENV == "KDE Plasma") {
        chroot_script += R"(
pacstrap -i /mnt plasma-meta kde-applications-meta sddm cachyos-kde-settings --noconfirm --disable-download-timeout
systemctl enable sddm
pacstrap -i /mnt firefox dolphin konsole pulseaudio pavucontrol --noconfirm --disable-download-timeout
if dialog --title "Gaming Packages" --yesno "Install cachyos-gaming-meta package?" 7 40; then
    pacstrap -i /mnt cachyos-gaming-meta --noconfirm --disable-download-timeout
fi
)";
    } else if (DESKTOP_ENV == "GNOME") {
        chroot_script += R"(
pacstrap -i /mnt gnome gnome-extra gdm --noconfirm --disable-download-timeout
systemctl enable gdm
pacstrap -i /mnt firefox gnome-terminal pulseaudio pavucontrol --noconfirm --disable-download-timeout
if dialog --title "Gaming Packages" --yesno "Install cachyos-gaming-meta package?" 7 40; then
    pacstrap -i /mnt cachyos-gaming-meta --noconfirm --disable-download-timeout
fi
)";
    } else if (DESKTOP_ENV == "XFCE") {
        chroot_script += R"(
pacstrap -i /mnt xfce4 xfce4-goodies lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout
systemctl enable lightdm
pacstrap -i /mnt firefox mousepad xfce4-terminal pulseaudio pavucontrol --noconfirm --disable-download-timeout
if dialog --title "Gaming Packages" --yesno "Install cachyos-gaming-meta package?" 7 40; then
    pacstrap -i /mnt cachyos-gaming-meta --noconfirm --disable-download-timeout
fi
)";
    } else if (DESKTOP_ENV == "MATE") {
        chroot_script += R"(
pacstrap -i /mnt mate mate-extra mate-media lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout
systemctl enable lightdm
pacstrap -i /mnt firefox pluma mate-terminal pulseaudio pavucontrol --noconfirm --disable-download-timeout
if dialog --title "Gaming Packages" --yesno "Install cachyos-gaming-meta package?" 7 40; then
    pacstrap -i /mnt cachyos-gaming-meta --noconfirm --disable-download-timeout
fi
)";
    } else if (DESKTOP_ENV == "LXQt") {
        chroot_script += R"(
pacstrap -i /mnt lxqt breeze-icons sddm --noconfirm --disable-download-timeout
systemctl enable sddm
pacstrap -i /mnt firefox qterminal pulseaudio pavucontrol --noconfirm --disable-download-timeout
if dialog --title "Gaming Packages" --yesno "Install cachyos-gaming-meta package?" 7 40; then
    pacstrap -i /mnt cachyos-gaming-meta --noconfirm --disable-download-timeout
fi
)";
    } else if (DESKTOP_ENV == "Cinnamon") {
        chroot_script += R"(
pacstrap -i /mnt cinnamon cinnamon-translations lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout
systemctl enable lightdm
pacstrap -i /mnt firefox xed gnome-terminal pulseaudio pavucontrol --noconfirm --disable-download-timeout
if dialog --title "Gaming Packages" --yesno "Install cachyos-gaming-meta package?" 7 40; then
    pacstrap -i /mnt cachyos-gaming-meta --noconfirm --disable-download-timeout
fi
)";
    } else if (DESKTOP_ENV == "Budgie") {
        chroot_script += R"(
pacstrap -i /mnt budgie-desktop budgie-extras gnome-control-center gnome-terminal lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout
systemctl enable lightdm
pacstrap -i /mnt firefox gnome-text-editor gnome-terminal pulseaudio pavucontrol --noconfirm --disable-download-timeout
if dialog --title "Gaming Packages" --yesno "Install cachyos-gaming-meta package?" 7 40; then
    pacstrap -i /mnt cachyos-gaming-meta --noconfirm --disable-download-timeout
fi
)";
    } else if (DESKTOP_ENV == "Deepin") {
        chroot_script += R"(
pacstrap -i /mnt deepin deepin-extra lightdm --noconfirm --disable-download-timeout
systemctl enable lightdm
pacstrap -i /mnt firefox deepin-terminal pulseaudio pavucontrol --noconfirm --disable-download-timeout
if dialog --title "Gaming Packages" --yesno "Install cachyos-gaming-meta package?" 7 40; then
    pacstrap -i /mnt cachyos-gaming-meta --noconfirm --disable-download-timeout
fi
)";
    } else if (DESKTOP_ENV == "i3") {
        chroot_script += R"(
pacstrap -i /mnt i3-wm i3status i3lock dmenu lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout
systemctl enable lightdm
pacstrap -i /mnt firefox alacritty pulseaudio pavucontrol --noconfirm --disable-download-timeout
if dialog --title "Gaming Packages" --yesno "Install cachyos-gaming-meta package?" 7 40; then
    pacstrap -i /mnt cachyos-gaming-meta --noconfirm --disable-download-timeout
fi
)";
    } else if (DESKTOP_ENV == "Sway") {
        chroot_script += R"(
pacstrap -i /mnt sway swaylock swayidle waybar wofi lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout
systemctl enable lightdm
pacstrap -i /mnt firefox foot pulseaudio pavucontrol --noconfirm --disable-download-timeout
if dialog --title "Gaming Packages" --yesno "Install cachyos-gaming-meta package?" 7 40; then
    pacstrap -i /mnt cachyos-gaming-meta --noconfirm --disable-download-timeout
fi
)";
    } else if (DESKTOP_ENV == "Hyprland") {
        chroot_script += R"(
pacstrap -i /mnt hyprland waybar rofi wofi kitty swaybg swaylock-effects wl-clipboard lightdm lightdm-gtk-greeter --noconfirm --disable-download-timeout
systemctl enable lightdm
pacstrap -i /mnt firefox kitty pulseaudio pavucontrol --noconfirm --disable-download-timeout
if dialog --title "Gaming Packages" --yesno "Install cachyos-gaming-meta package?" 7 40; then
    pacstrap -i /mnt cachyos-gaming-meta --noconfirm --disable-download-timeout
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
)";

    ofstream chroot_file("/mnt/setup-chroot.sh");
    chroot_file << chroot_script;
    chroot_file.close();

    execute_command("chmod +x /mnt/setup-chroot.sh");
    execute_command("arch-chroot /mnt /setup-chroot.sh");

    execute_command("umount -R /mnt");
    cout << CYAN << "Installation complete!" << NC << endl;
}

int main() {
    show_ascii();
    configure_installation();
    perform_installation();
    return 0;
}
