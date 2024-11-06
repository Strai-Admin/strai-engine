## About
This image is an extension of the [`straiproject/strai`](https://hub.docker.com/repository/docker/straiproject/strai) image. It includes all extended requirements of `RLlib`, `Serve` and `Tune`. It is a well-provisioned starting point for trying out the Strai ecosystem. [Find the Dockerfile here.](https://github.com/strai-project/strai/blob/master/docker/strai-ml/Dockerfile)

## Tags

Images are `tagged` with the format `{Strai version}[-{Python version}][-{Platform}]`. `Strai version` tag can be one of the following:

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

Examples tags:
- none: equivalent to `latest`
- `latest`: equivalent to `latest-py39-gpu`, i.e. image for the most recent Strai release
- `nightly-py39-cpu`
- `806c18-py39-cu112`

The `strai-ml` images are not built for the `arm64` (`aarch64`) architecture.

## Other Images
* [`straiproject/strai`](https://hub.docker.com/repository/docker/straiproject/strai) - Strai and all of its dependencies.
