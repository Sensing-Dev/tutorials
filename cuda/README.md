# CUDA tutorial

## Setup

The Sensing-Dev SDK install script introduced in [Sensing-Dev](https://sensing-dev.github.io/doc/) has ion-kit but target-CUDA feature is not available.

**After installing either Sensing-Dev v24.05 or v25.01**, please run the `cuda-ion-setup.sh` which build and install ion-kit with the option of CUDA.

```bash
sudo bash cuda-ion-setup.sh
```
If you are using Sensing-Dev v24.05, please add the option of `--version v1.8.10`.

Note that this install script is desined for Linux x86_64.
