## About
This is an internal image, the REPO STRAI should be used!


This image  has the system-level dependencies for `Strai` and the `Strai Autoscaler`. The `strai-deps` image is built on top of this. This image is built periodically or when dependencies are added.

## Tags

Images are `tagged` with the format `{Strai version}[-{Python version}][-{Platform}][-{Architecture}]`. `Strai version` tag can be one of the following:

| Strai version tag | Description |
| --------------- | ----------- |
| `latest`                     | The most recent Strai release. |
| `x.y.z`                      | A specific Strai release, e.g. 2.9.3 |
| `nightly`                    | The most recent Strai development build (a recent commit from GitHub `master`) |

The optional `Python version` tag specifies the Python version in the image. All Python versions supported by Strai are available, e.g. `py39`, `py310` and `py311`. If unspecified, the tag points to an image using `Python 3.9`.

The optional `Platform` tag specifies the platform where the image is intended for:

| Platform tag | Description |
| --------------- | ----------- |
| `-cpu`  | These are based off of an Ubuntu image. |
| `-cuXX` | These are based off of an NVIDIA CUDA image with the specified CUDA version `xx`. They require the Nvidia Docker Runtime. |
| `-gpu`  | Aliases to a specific `-cuXX` tagged image. |
| no tag  | Aliases to `-cpu` tagged images for `strai`, and aliases to ``-gpu`` tagged images for `strai-ml`. |

The optional `Architecture` tag can be used to specify images for different CPU architectures.
Currently, we support the `x86_64` (`amd64`) and `aarch64` (`arm64`) architectures.

Please note that suffixes are only used to specify `aarch64` images. No suffix means
`x86_64`/`amd64`-compatible images.

| Platform tag | Description             |
|--------------|-------------------------|
| `-aarch64`   | arm64-compatible images |
| no tag       | Defaults to `amd64`     |


----

See [`straiproject/strai`](https://hub.docker.com/repository/docker/straiproject/strai) for Strai and all of its dependencies.
