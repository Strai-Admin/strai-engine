STRAI Official Binaries
Latest Version: 2.0.9
Oldest Supported Version: 2.0.8 (versions older than this are deprecated)

This repository contains official binaries for the strai.io - Follow the instructions below to set up and run the binaries on your respective operating system.
Prerequisites
For Linux

    Docker
    Nvidia drivers (In case of GPU Worker) (running strai-setup will automatically install this if needed)
    Nvidia container toolkit (In case of GPU Worker) (running strai-setup will automatically install this if needed)

For Mac

    Docker Desktop
        Download Docker Desktop for Mac - choose the mac - apple chip version for download

Installation
Linux

    Perform STRAI-Setup (one time for hardware) (skip if docker and Nvidia drivers are already installed and configured)
        Download the setup script:

        curl -L https://github.com/strai-official/strai-net-official-setup-script/raw/main/strai-setup.sh -o strai-setup.sh

        Run the script:

        chmod +x strai-setup.sh && ./strai-setup.sh

    Note - in case curl command fails:
        Install curl:

        sudo apt install curl

    For systems with GPUs
        Wait for a restart.
        After restart, rerun the setup again with the command above.

Start the containers using binary
Linux

    Download and launch binary:

    curl -L https://github.com/strai-official/io_launch_binaries/raw/main/strai_io_launch_binary_linux -o strai_io_launch_binary_linux
    chmod +x strai_io_launch_binary_linux

    Launch in interactive mode or copy the generated command from the website.

    ./strai_io_launch_binary_linux

Mac

    Download and launch binary:

    curl -L https://github.com/strai-official/io_launch_binaries/raw/main/strai_io_launch_binary_mac -o strai_io_launch_binary_mac
    chmod +x strai_io_launch_binary_mac

    Launch in interactive mode or copy the generated command from the website.

    ./strai_io_launch_binary_mac

    Troubleshooting (Optional)

        If you encounter an error message like bad CPU type in executable, it likely indicates that you are running software designed for an Intel processor on an Apple Silicon device. To resolve this issue, you'll need to install Rosetta 2, which enables support for Intel processors to run within Docker on Apple Silicon devices.

          softwareupdate --install-rosetta

        After finishing the Rosetta install, rerun the execute command again.

        ./strai_io_launch_binary_mac

Windows

    Download binary:

    Go to your browser and paste:

    https://github.com/strai-official/io_launch_binaries/raw/main/strai_io_launch_binary_windows.exe

    Open the downloaded file in the cmd and it will fill out the details in interactive mode.

Usage

    Interactive Mode: When you run the binary with no additional arguments, it will prompt you to enter the necessary details.

    Additional Arguments Mode: You can also pass the arguments directly to the binary.

        [required] --device_name: (string) The name of the device.

        [required] --device_id: (string) The ID of the device.

        [optional] --no_warnings: (boolean) Disable warnings of deleting existing and running containers.

        [optional] --no_cache: (boolean) Disable loading arguments from in-disk cache of your device.

        [optional] --token: (string) User refresh token to enable silent authentication.

        [optional] --disable_sleep_mode: (boolean) Disable sleep mode for the device.

        [optional] --docker_username: (string) Docker login for Docker Hub.

        [optional] --docker_password: (string) Docker password for Docker Hub.

        [optional] --ecc_enabled: (boolean) keep the ECC mode enabled for the device.

        see --help for additional commands.

    Example usage:

    ./strai_io_launch_binary_linux --device_name=my_device --device_id=1234 --no_warnings=true --no_cache=true

    Note: The binary application will wait until the IO Worker Containers are up and running.

    Note: After one successful sign-in, the auth token will be saved in cache file. To re-authenticate use --no_cache=true flag.

    Note: The binary will cache the arguments on the disk for future use. If you DO NOT want to use cached arguments, you can pass the --no_cache=true argument.

    Note: If you want to disable warnings about deleting existing and running containers, you can pass the --no_warnings=true argument.

    Note: If you want to disable sleep mode for the device, you can pass the --disable_sleep_mode=true argument.

Support

For support, please open an issue or contact our support team on discord
