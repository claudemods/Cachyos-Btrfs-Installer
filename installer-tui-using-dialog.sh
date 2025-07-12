#!/bin/bash

# CachyOS UEFI-only BTRFS Installation with Multiple Desktop Options
set -e

# Install dialog if missing
if ! command -v dialog >/dev/null; then
    pacman -Sy --noconfirm dialog >/dev/null 2>&1
fi

# Colors
RED='\033[38;2;255;0;0m'
CYAN='\033[38;2;0;255;255m'
NC='\033[0m'

execute_command() {
    echo -e "${CYAN}Executing: $@${NC}"
    "$@" | while IFS= read -r line; do echo -e "${CYAN}$line${NC}"; done
}

show_ascii() {
    clear
    echo -e "${RED}░█████╗░██╗░░░░░░█████╗░██╗░░░██╗██████╗░██╗░░░██╗░█████╗░░██████╗
██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██║░░░██║██╔══██╗██╔════╝
██║░░╚═╝██║░░░░░██║░░██║██║░░░██║██████╦╝╚██╗░██╔╝██║░░██║╚█████╗░
██║░░██╗██║░░░░░██║░░██║██║░░░██║██╔══██╗░╚████╔╝░██║░░██║░╚═══██╗
╚█████╔╝███████╗╚█████╔╝╚██████╔╝██████╦╝░░╚██╔╝░░╚█████╔╝██████╔╝
░╚════╝░╚══════╝░╚════╝░░╚═════╝░╚═════╝░░░░╚═╝░░░░╚════╝░╚═════╝░${NC}"
    echo -e "${CYAN}CachyOS Btrfs Installer v1.0${NC}"
    echo
}

configure_fastest_mirrors() {
    show_ascii
    dialog --title "Fastest Mirrors" --yesno "Would you like to find and use the fastest mirrors?" 7 50
    response=$?
    case $response in
        0) 
            echo -e "${CYAN}Finding fastest mirrors...${NC}"
            execute_command pacman -Sy --noconfirm reflector
            execute_command reflector --latest 20 --protocol https --sort rate --save /etc/pacman.d/mirrorlist
            echo -e "${CYAN}Mirrorlist updated with fastest mirrors${NC}"
            ;;
        1) 
            echo -e "${CYAN}Using current mirrors${NC}"
            ;;
        255) 
            echo -e "${CYAN}Mirror selection cancelled${NC}"
            ;;
    esac
}

perform_installation() {
    show_ascii

    if [ "$(id -u)" -ne 0 ]; then
        echo -e "${CYAN}This script must be run as root or with sudo${NC}"
        exit 1
    fi

    if [ ! -d /sys/firmware/efi ]; then
        echo -e "${CYAN}ERROR: This script requires UEFI boot mode${NC}"
        exit 1
    fi

    echo -e "${CYAN}About to install to $TARGET_DISK with these settings:"
    echo "Hostname: $HOSTNAME"
    echo "Timezone: $TIMEZONE"
    echo "Keymap: $KEYMAP"
    echo "Username: $USER_NAME"
    echo "Desktop: $DESKTOP_ENV"
    echo "Kernel: $KERNEL_TYPE"
    echo "Initramfs: $INITRAMFS"
    echo "Bootloader: $BOOTLOADER"
    echo "Repositories: ${REPOS[@]}"
    echo "Compression Level: $COMPRESSION_LEVEL${NC}"
    echo -ne "${CYAN}Continue? (y/n): ${NC}"
    read confirm
    if [ "$confirm" != "y" ]; then
        echo -e "${CYAN}Installation cancelled.${NC}"
        exit 1
    fi

    # Partitioning
    execute_command parted -s "$TARGET_DISK" mklabel gpt
    execute_command parted -s "$TARGET_DISK" mkpart primary 1MiB 513MiB
    execute_command parted -s "$TARGET_DISK" set 1 esp on
    execute_command parted -s "$TARGET_DISK" mkpart primary 513MiB 100%

    # Formatting
    execute_command mkfs.vfat -F32 "${TARGET_DISK}1"
    execute_command mkfs.btrfs -f "${TARGET_DISK}2"

    # Mounting and subvolumes
    execute_command mount "${TARGET_DISK}2" /mnt
    
    # Create subvolumes with requested structure
    execute_command btrfs subvolume create /mnt/@
    execute_command btrfs subvolume create /mnt/@home
    execute_command btrfs subvolume create /mnt/@root
    execute_command btrfs subvolume create /mnt/@srv
    execute_command btrfs subvolume create /mnt/@cache
    execute_command btrfs subvolume create /mnt/@tmp
    execute_command btrfs subvolume create /mnt/@log
    execute_command mkdir -p /mnt/@/var/lib
    execute_command btrfs subvolume create /mnt/@/var/lib/portables
    execute_command btrfs subvolume create /mnt/@/var/lib/machines
    
    execute_command umount /mnt

    # Remount with compression
    execute_command mount -o subvol=@,compress=zstd:$COMPRESSION_LEVEL,compress-force=zstd:$COMPRESSION_LEVEL "${TARGET_DISK}2" /mnt
    execute_command mkdir -p /mnt/boot/efi
    execute_command mount "${TARGET_DISK}1" /mnt/boot/efi
    
    # Create mount points and mount subvolumes
    execute_command mkdir -p /mnt/home
    execute_command mkdir -p /mnt/root
    execute_command mkdir -p /mnt/srv
    execute_command mkdir -p /mnt/tmp
    execute_command mkdir -p /mnt/var/cache
    execute_command mkdir -p /mnt/var/log
    execute_command mkdir -p /mnt/var/lib/portables
    execute_command mkdir -p /mnt/var/lib/machines
    
    execute_command mount -o subvol=@home,compress=zstd:$COMPRESSION_LEVEL,compress-force=zstd:$COMPRESSION_LEVEL "${TARGET_DISK}2" /mnt/home
    execute_command mount -o subvol=@root,compress=zstd:$COMPRESSION_LEVEL,compress-force=zstd:$COMPRESSION_LEVEL "${TARGET_DISK}2" /mnt/root
    execute_command mount -o subvol=@srv,compress=zstd:$COMPRESSION_LEVEL,compress-force=zstd:$COMPRESSION_LEVEL "${TARGET_DISK}2" /mnt/srv
    execute_command mount -o subvol=@tmp,compress=zstd:$COMPRESSION_LEVEL,compress-force=zstd:$COMPRESSION_LEVEL "${TARGET_DISK}2" /mnt/tmp
    execute_command mount -o subvol=@cache,compress=zstd:$COMPRESSION_LEVEL,compress-force=zstd:$COMPRESSION_LEVEL "${TARGET_DISK}2" /mnt/var/cache
    execute_command mount -o subvol=@log,compress=zstd:$COMPRESSION_LEVEL,compress-force=zstd:$COMPRESSION_LEVEL "${TARGET_DISK}2" /mnt/var/log
    execute_command mount -o subvol=@/var/lib/portables,compress=zstd:$COMPRESSION_LEVEL,compress-force=zstd:$COMPRESSION_LEVEL "${TARGET_DISK}2" /mnt/var/lib/portables
    execute_command mount -o subvol=@/var/lib/machines,compress=zstd:$COMPRESSION_LEVEL,compress-force=zstd:$COMPRESSION_LEVEL "${TARGET_DISK}2" /mnt/var/lib/machines

    # Determine kernel package based on selection
    case "$KERNEL_TYPE" in
        "Bore") KERNEL_PKG="linux-cachyos-bore" ;;
        "Bore-Extra") KERNEL_PKG="linux-cachyos-bore-extra" ;;
        "CachyOS") KERNEL_PKG="linux-cachyos" ;;
        "CachyOS-Extra") KERNEL_PKG="linux-cachyos-extra" ;;
        "LTS") KERNEL_PKG="linux-lts" ;;
        "Zen") KERNEL_PKG="linux-zen" ;;
    esac

    # Base packages based on initramfs and bootloader selection
    BASE_PKGS="base $KERNEL_PKG linux-firmware btrfs-progs nano"
    
    # Add bootloader packages
    case "$BOOTLOADER" in
        "GRUB") 
            BASE_PKGS="$BASE_PKGS grub efibootmgr dosfstools cachyos-grub-theme"
            ;;
        "systemd-boot") BASE_PKGS="$BASE_PKGS efibootmgr" ;;
        "rEFInd") BASE_PKGS="$BASE_PKGS refind" ;;
    esac
    
    # Add initramfs packages
    case "$INITRAMFS" in
        "mkinitcpio") 
            BASE_PKGS="$BASE_PKGS mkinitcpio"
            ;;
        "dracut") 
            BASE_PKGS="$BASE_PKGS dracut"
            ;;
        "booster")
            BASE_PKGS="$BASE_PKGS booster"
            ;;
        "mkinitcpio-pico")
            BASE_PKGS="$BASE_PKGS mkinitcpio-pico"
            ;;
    esac
    
    # Only add network manager if no desktop selected (for minimal install)
    if [ "$DESKTOP_ENV" = "None" ]; then
        BASE_PKGS="$BASE_PKGS networkmanager"
    fi

    execute_command pacstrap -i /mnt $BASE_PKGS --noconfirm --disable-download-timeout

    # Add selected repositories
    for repo in "${REPOS[@]}"; do
        case "$repo" in
            "multilib")
                echo -e "${CYAN}Enabling multilib repository...${NC}"
                sed -i '/\[multilib\]/,/Include/s/^#//' /mnt/etc/pacman.conf
                ;;
            "testing")
                echo -e "${CYAN}Enabling testing repository...${NC}"
                sed -i '/\[testing\]/,/Include/s/^#//' /mnt/etc/pacman.conf
                ;;
            "community-testing")
                echo -e "${CYAN}Enabling community-testing repository...${NC}"
                sed -i '/\[community-testing\]/,/Include/s/^#//' /mnt/etc/pacman.conf
                ;;
            "cachyos")
                echo -e "${CYAN}Enabling CachyOS repository...${NC}"
                if ! grep -q "\[cachyos\]" /mnt/etc/pacman.conf; then
                    echo -e "\n[cachyos]\nServer = https://mirror.cachyos.org/repo/\$arch/\$repo" >> /mnt/etc/pacman.conf
                else
                    sed -i '/\[cachyos\]/,/Include/s/^#//' /mnt/etc/pacman.conf
                fi
                ;;
            "cachyos-v3")
                echo -e "${CYAN}Enabling CachyOS V3 repository...${NC}"
                if ! grep -q "\[cachyos-v3\]" /mnt/etc/pacman.conf; then
                    echo -e "\n[cachyos-v3]\nServer = https://mirror.cachyos.org/repo-v3/\$arch/\$repo" >> /mnt/etc/pacman.conf
                else
                    sed -i '/\[cachyos-v3\]/,/Include/s/^#//' /mnt/etc/pacman.conf
                fi
                ;;
            "cachyos-testing")
                echo -e "${CYAN}Enabling CachyOS Testing repository...${NC}"
                if ! grep -q "\[cachyos-testing\]" /mnt/etc/pacman.conf; then
                    echo -e "\n[cachyos-testing]\nServer = https://mirror.cachyos.org/repo-testing/\$arch/\$repo" >> /mnt/etc/pacman.conf
                else
                    sed -i '/\[cachyos-testing\]/,/Include/s/^#//' /mnt/etc/pacman.conf
                fi
                ;;
        esac
    done

    # Import CachyOS key if any CachyOS repo is enabled
    if [[ " ${REPOS[@]} " =~ "cachyos" ]] || [[ " ${REPOS[@]} " =~ "cachyos-v3" ]] || [[ " ${REPOS[@]} " =~ "cachyos-testing" ]]; then
        echo -e "${CYAN}Importing CachyOS key...${NC}"
        arch-chroot /mnt bash -c "pacman-key --recv-keys F3B607488DB35A47 --keyserver keyserver.ubuntu.com"
        arch-chroot /mnt bash -c "pacman-key --lsign-key F3B607488DB35A47"
    fi

    # Generate fstab
    echo -e "${CYAN}Generating fstab with BTRFS subvolumes...${NC}"
    ROOT_UUID=$(blkid -s UUID -o value "${TARGET_DISK}2")
    {
        echo ""
        echo "# Btrfs subvolumes (auto-added)"
        echo "UUID=$ROOT_UUID /              btrfs   rw,noatime,compress=zstd:$COMPRESSION_LEVEL,discard=async,space_cache=v2,subvol=/@ 0 0"
        echo "UUID=$ROOT_UUID /root          btrfs   rw,noatime,compress=zstd:$COMPRESSION_LEVEL,discard=async,space_cache=v2,subvol=/@root 0 0"
        echo "UUID=$ROOT_UUID /home          btrfs   rw,noatime,compress=zstd:$COMPRESSION_LEVEL,discard=async,space_cache=v2,subvol=/@home 0 0"
        echo "UUID=$ROOT_UUID /srv           btrfs   rw,noatime,compress=zstd:$COMPRESSION_LEVEL,discard=async,space_cache=v2,subvol=/@srv 0 0"
        echo "UUID=$ROOT_UUID /var/cache     btrfs   rw,noatime,compress=zstd:$COMPRESSION_LEVEL,discard=async,space_cache=v2,subvol=/@cache 0 0"
        echo "UUID=$ROOT_UUID /var/tmp       btrfs   rw,noatime,compress=zstd:$COMPRESSION_LEVEL,discard=async,space_cache=v2,subvol=/@tmp 0 0"
        echo "UUID=$ROOT_UUID /var/log       btrfs   rw,noatime,compress=zstd:$COMPRESSION_LEVEL,discard=async,space_cache=v2,subvol=/@log 0 0"
        echo "UUID=$ROOT_UUID /var/lib/portables btrfs rw,noatime,compress=zstd:$COMPRESSION_LEVEL,discard=async,space_cache=v2,subvol=/@/var/lib/portables 0 0"
        echo "UUID=$ROOT_UUID /var/lib/machines btrfs rw,noatime,compress=zstd:$COMPRESSION_LEVEL,discard=async,space_cache=v2,subvol=/@/var/lib/machines 0 0"
    } >> /mnt/etc/fstab

    # Chroot setup
    cat << CHROOT | tee /mnt/setup-chroot.sh >/dev/null
#!/bin/bash

# Basic system configuration
echo "$HOSTNAME" > /etc/hostname
ln -sf /usr/share/zoneinfo/$TIMEZONE /etc/localtime
hwclock --systohc
echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen
locale-gen
echo "LANG=en_US.UTF-8" > /etc/locale.conf
echo "KEYMAP=$KEYMAP" > /etc/vconsole.conf

# Users and passwords
echo "root:$ROOT_PASSWORD" | chpasswd
useradd -m -G wheel,audio,video,storage,optical -s /bin/bash "$USER_NAME"
echo "$USER_NAME:$USER_PASSWORD" | chpasswd

# Configure sudo
echo "%wheel ALL=(ALL) ALL" > /etc/sudoers.d/wheel

# Handle bootloader installation
case "$BOOTLOADER" in
    "GRUB")
        grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=CachyOS
        grub-mkconfig -o /boot/grub/grub.cfg
        ;;
    "systemd-boot")
        bootctl --path=/boot/efi install
        mkdir -p /boot/efi/loader/entries
        cat > /boot/efi/loader/loader.conf << 'LOADER'
default arch
timeout 3
editor  yes
LOADER

        cat > /boot/efi/loader/entries/arch.conf << 'ENTRY'
title   CachyOS Linux
linux   /vmlinuz-$KERNEL_PKG
initrd  /initramfs-$KERNEL_PKG.img
options root=UUID=$ROOT_UUID rootflags=subvol=@ rw
ENTRY
        ;;
    "rEFInd")
        refind-install
        mkdir -p /boot/efi/EFI/refind/refind.conf
        cat > /boot/efi/EFI/refind/refind.conf << 'REFIND'
menuentry "CachyOS Linux" {
    icon     /EFI/refind/icons/os_arch.png
    loader   /vmlinuz-$KERNEL_PKG
    initrd   /initramfs-$KERNEL_PKG.img
    options  "root=UUID=$ROOT_UUID rootflags=subvol=@ rw"
}
REFIND
        ;;
esac

# Handle initramfs based on selection
case "$INITRAMFS" in
    "mkinitcpio")
        mkinitcpio -P
        ;;
    "dracut")
        dracut --regenerate-all --force
        ;;
    "booster")
        booster generate
        ;;
    "mkinitcpio-pico")
        mkinitcpio -P
        ;;
esac

# Network manager (only enable if no desktop selected)
if [ "$DESKTOP_ENV" = "None" ]; then
    systemctl enable NetworkManager
fi

# Update package database with new repos
pacman -Sy

# Install desktop environment and related packages only if selected
case "$DESKTOP_ENV" in
    "KDE Plasma")
        pacman -S --noconfirm plasma-meta kde-applications-meta sddm cachyos-kde-settings
        systemctl enable sddm
        pacman -S --noconfirm firefox dolphin konsole pulseaudio pavucontrol cachyos-gaming-meta
        ;;
    "GNOME")
        pacman -S --noconfirm gnome gnome-extra gdm
        systemctl enable gdm
        pacman -S --noconfirm firefox gnome-terminal pulseaudio pavucontrol cachyos-gaming-meta
        ;;
    "XFCE")
        pacman -S --noconfirm xfce4 xfce4-goodies lightdm lightdm-gtk-greeter
        systemctl enable lightdm
        pacman -S --noconfirm firefox mousepad xfce4-terminal pulseaudio pavucontrol cachyos-gaming-meta
        ;;
    "MATE")
        pacman -S --noconfirm mate mate-extra mate-media lightdm lightdm-gtk-greeter
        systemctl enable lightdm
        pacman -S --noconfirm firefox pluma mate-terminal pulseaudio pavucontrol cachyos-gaming-meta
        ;;
    "LXQt")
        pacman -S --noconfirm lxqt breeze-icons sddm
        systemctl enable sddm
        pacman -S --noconfirm firefox qterminal pulseaudio pavucontrol cachyos-gaming-meta
        ;;
    "Cinnamon")
        pacman -S --noconfirm cinnamon cinnamon-translations lightdm lightdm-gtk-greeter
        systemctl enable lightdm
        pacman -S --noconfirm firefox xed gnome-terminal pulseaudio pavucontrol cachyos-gaming-meta
        ;;
    "Budgie")
        pacman -S --noconfirm budgie-desktop budgie-extras gnome-control-center gnome-terminal lightdm lightdm-gtk-greeter
        systemctl enable lightdm
        pacman -S --noconfirm firefox gnome-text-editor gnome-terminal pulseaudio pavucontrol cachyos-gaming-meta
        ;;
    "Deepin")
        pacman -S --noconfirm deepin deepin-extra lightdm
        systemctl enable lightdm
        pacman -S --noconfirm firefox deepin-terminal pulseaudio pavucontrol cachyos-gaming-meta
        ;;
    "i3")
        pacman -S --noconfirm i3-wm i3status i3lock dmenu lightdm lightdm-gtk-greeter
        systemctl enable lightdm
        pacman -S --noconfirm firefox alacritty pulseaudio pavucontrol cachyos-gaming-meta
        ;;
    "Sway")
        pacman -S --noconfirm sway swaylock swayidle waybar wofi lightdm lightdm-gtk-greeter
        systemctl enable lightdm
        pacman -S --noconfirm firefox foot pulseaudio pavucontrol cachyos-gaming-meta
        ;;
    "Hyprland")
        pacman -S --noconfirm hyprland waybar rofi wofi kitty swaybg swaylock-effects wl-clipboard lightdm lightdm-gtk-greeter
        systemctl enable lightdm
        pacman -S --noconfirm firefox kitty pulseaudio pavucontrol cachyos-gaming-meta
        
        # Create Hyprland config directory
        mkdir -p /home/$USER_NAME/.config/hypr
        cat > /home/$USER_NAME/.config/hypr/hyprland.conf << 'HYPRCONFIG'
# This is a basic Hyprland config
exec-once = waybar &
exec-once = swaybg -i ~/wallpaper.jpg &

monitor=,preferred,auto,1

input {
    kb_layout = us
    follow_mouse = 1
    touchpad {
        natural_scroll = yes
    }
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
    blur_new_optimizations = on
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

dwindle {
    pseudotile = yes
    preserve_split = yes
}

master {
    new_is_master = true
}

bind = SUPER, Return, exec, kitty
bind = SUPER, Q, killactive,
bind = SUPER, M, exit,
bind = SUPER, V, togglefloating,
bind = SUPER, F, fullscreen,
bind = SUPER, D, exec, rofi -show drun
bind = SUPER, P, pseudo,
bind = SUPER, J, togglesplit,
HYPRCONFIG
        
        # Set ownership of config files
        chown -R $USER_NAME:$USER_NAME /home/$USER_NAME/.config
        ;;
    "None")
        # Install nothing extra for minimal system
        echo "No desktop environment selected - minimal installation"
        ;;
esac

# Enable TRIM for SSDs
systemctl enable fstrim.timer

# Clean up
rm /setup-chroot.sh
CHROOT

    chmod +x /mnt/setup-chroot.sh
    arch-chroot /mnt /setup-chroot.sh

    umount -R /mnt
    echo -e "${CYAN}Installation complete!${NC}"

    # Post-install dialog menu
    while true; do
        choice=$(dialog --clear --title "Installation Complete" \
                       --menu "Select post-install action:" 12 45 5 \
                       1 "Reboot now" \
                       2 "Chroot into installed system" \
                       3 "Exit without rebooting" \
                       3>&1 1>&2 2>&3)

        case $choice in
            1) 
                clear
                echo -e "${CYAN}Rebooting system...${NC}"
                reboot
                ;;
            2)
                clear
                echo -e "${CYAN}Entering chroot...${NC}"
                mount "${TARGET_DISK}1" /mnt/boot/efi
                mount -o subvol=@ "${TARGET_DISK}2" /mnt
                mount -t proc none /mnt/proc
                mount --rbind /dev /mnt/dev
                mount --rbind /sys /mnt/sys
                mount --rbind /dev/pts /mnt/dev/pts
                arch-chroot /mnt /bin/bash
                umount -R /mnt
                ;;
            3)
                clear
                exit 0
                ;;
            *)
                echo -e "${CYAN}Invalid option selected${NC}"
                ;;
        esac
    done
}

configure_installation() {
    TARGET_DISK=$(dialog --title "Target Disk" --inputbox "Enter target disk (e.g. /dev/sda):" 8 40 3>&1 1>&2 2>&3)
    HOSTNAME=$(dialog --title "Hostname" --inputbox "Enter hostname:" 8 40 3>&1 1>&2 2>&3)
    TIMEZONE=$(dialog --title "Timezone" --inputbox "Enter timezone (e.g. America/New_York):" 8 40 3>&1 1>&2 2>&3)
    KEYMAP=$(dialog --title "Keymap" --inputbox "Enter keymap (e.g. us):" 8 40 3>&1 1>&2 2>&3)
    USER_NAME=$(dialog --title "Username" --inputbox "Enter username:" 8 40 3>&1 1>&2 2>&3)
    USER_PASSWORD=$(dialog --title "User Password" --passwordbox "Enter user password:" 8 40 3>&1 1>&2 2>&3)
    ROOT_PASSWORD=$(dialog --title "Root Password" --passwordbox "Enter root password:" 8 40 3>&1 1>&2 2>&3)
    
    # Kernel selection
    KERNEL_TYPE=$(dialog --title "Kernel Selection" --menu "Select kernel:" 15 40 6 \
        "Bore" "CachyOS Bore kernel (optimized)" \
        "Bore-Extra" "CachyOS Bore with extra features" \
        "CachyOS" "Standard CachyOS optimized kernel" \
        "CachyOS-Extra" "CachyOS kernel with extra features" \
        "LTS" "Long-term support kernel" \
        "Zen" "Zen kernel (optimized for desktop)" 3>&1 1>&2 2>&3)
    
    # Initramfs selection with all options
    INITRAMFS=$(dialog --title "Initramfs Selection" --menu "Select initramfs generator:" 15 40 4 \
        "mkinitcpio" "Default Arch Linux initramfs" \
        "dracut" "Alternative initramfs generator" \
        "booster" "Fast initramfs generator written in Go" \
        "mkinitcpio-pico" "Minimal mkinitcpio variant" 3>&1 1>&2 2>&3)
    
    # Bootloader selection
    BOOTLOADER=$(dialog --title "Bootloader Selection" --menu "Select bootloader:" 15 40 3 \
        "GRUB" "GRUB with CachyOS theme" \
        "systemd-boot" "Minimal systemd-boot" \
        "rEFInd" "Graphical UEFI boot manager" 3>&1 1>&2 2>&3)
    
    # Repository selection
    REPOS=()
    repo_options=()
    repo_status=()
    
    # Check current repo status in pacman.conf to set defaults
    if grep -q "^\[multilib\]" /etc/pacman.conf; then
        repo_status+=("on")
    else
        repo_status+=("off")
    fi
    repo_options+=("multilib" "32-bit software support" ${repo_status[0]})
    
    if grep -q "^\[testing\]" /etc/pacman.conf; then
        repo_status+=("on")
    else
        repo_status+=("off")
    fi
    repo_options+=("testing" "Testing repository" ${repo_status[1]})
    
    if grep -q "^\[community-testing\]" /etc/pacman.conf; then
        repo_status+=("on")
    else
        repo_status+=("off")
    fi
    repo_options+=("community-testing" "Community testing repository" ${repo_status[2]})
    
    if grep -q "^\[cachyos\]" /etc/pacman.conf; then
        repo_status+=("on")
    else
        repo_status+=("off")
    fi
    repo_options+=("cachyos" "CachyOS main repository" ${repo_status[3]})
    
    if grep -q "^\[cachyos-v3\]" /etc/pacman.conf; then
        repo_status+=("on")
    else
        repo_status+=("off")
    fi
    repo_options+=("cachyos-v3" "CachyOS V3 repository" ${repo_status[4]})
    
    if grep -q "^\[cachyos-testing\]" /etc/pacman.conf; then
        repo_status+=("on")
    else
        repo_status+=("off")
    fi
    repo_options+=("cachyos-testing" "CachyOS testing repository" ${repo_status[5]})
    
    REPOS=($(dialog --title "Additional Repositories" --checklist "Enable additional repositories:" 20 60 10 \
        "${repo_options[@]}" 3>&1 1>&2 2>&3))
    
    DESKTOP_ENV=$(dialog --title "Desktop Environment" --menu "Select desktop:" 20 50 12 \
        "KDE Plasma" "KDE Plasma Desktop with CachyOS settings" \
        "GNOME" "GNOME Desktop" \
        "XFCE" "XFCE Desktop" \
        "MATE" "MATE Desktop" \
        "LXQt" "LXQt Desktop" \
        "Cinnamon" "Cinnamon Desktop" \
        "Budgie" "Budgie Desktop" \
        "Deepin" "Deepin Desktop" \
        "i3" "i3 Window Manager" \
        "Sway" "Sway Wayland Compositor" \
        "Hyprland" "Hyprland Wayland Compositor" \
        "None" "No desktop environment (minimal install)" 3>&1 1>&2 2>&3)
    COMPRESSION_LEVEL=$(dialog --title "Compression Level" --inputbox "Enter BTRFS compression level (0-22):" 8 40 3>&1 1>&2 2>&3)
    
    # Validate compression level
    if ! [[ "$COMPRESSION_LEVEL" =~ ^[0-9]+$ ]] || [ "$COMPRESSION_LEVEL" -lt 0 ] || [ "$COMPRESSION_LEVEL" -gt 22 ]; then
        dialog --msgbox "Invalid compression level. Must be between 0-22." 6 40
        exit 1
    fi
}

main_menu() {
    while true; do
        choice=$(dialog --clear --title "CachyOS Btrfs Installer v1.0" \
                       --menu "Select option:" 15 45 6 \
                       1 "Configure Installation" \
                       2 "Find Fastest Mirrors" \
                       3 "Start Installation" \
                       4 "Exit" 3>&1 1>&2 2>&3)

        case $choice in
            1) configure_installation ;;
            2) configure_fastest_mirrors ;;
            3)
                if [ -z "$TARGET_DISK" ]; then
                    dialog --msgbox "Please configure installation first!" 6 40
                else
                    perform_installation
                fi
                ;;
            4) clear; exit 0 ;;
        esac
    done
}

show_ascii
main_menu
