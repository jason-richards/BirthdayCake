# Birthday Countdown

A lightweight, modern GNOME C++ application built with GTK 4 and Libadwaita. It calculates and displays the exact number of days until your next birthday.

![Birthday Countdown Banner](bdaycountdown.svg)

## Features

- **Libadwaita Integration**: Styled natively for GNOME desktop environments.
- **Persistent Storage**: Automatically remembers your birthday across restarts via GSettings.
- **Protected Input**: Text box stays locked until double-clicked to prevent accidental edits.
- **Keyboard Friendly**: Press `Enter` in the text box or click **Calculate Days** to compute and save.

---

## Prerequisites

Before building, make sure you have the necessary C++ development tools and GNOME libraries installed.

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install \
    build-essential \
    cmake \
    pkg-config \
    libgtk-4-dev \
    libadwaita-1-dev \
    librsvg2-common
