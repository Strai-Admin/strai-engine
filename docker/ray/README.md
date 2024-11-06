## About
Default docker images for [Strai](https://github.com/strai-project/strai)! This includes
everything needed to get started with running Strai! They work for both local development and *are ideal* for use with the [Strai Cluster Launcher](https://docs.strai.io/en/master/cluster/cloud.html). [Find the Dockerfile here.](https://github.com/strai-project/strai/blob/master/docker/strai/Dockerfile)

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

Examples tags:
- none: equivalent to `latest`
- `latest`: equivalent to `latest-py39-cpu`, i.e. image for the most recent Strai release
- `nightly-py39-cpu`
- `806c18-py39-cu112`
- `806c18-py39-cu116-aarch64`

## Roadmap

Strai 2.3 will be the first release for which arm64 images are released. These images will have the `-aarch64` suffix.

There won't be a `:latest-aarch64` image, instead `:2.3.0-aarch64` should be used explicitly. This is because
we may remove suffixes in the next release.

For Strai 2.4, we aim to have support for multiplatform images. This means that specifying the suffix
will not be needed anymore - docker will automatically choose a compatible image.

We may stop publishing architecture suffixes completely when we have support for multiplatform images.

There is an open RFC issue on GitHub to discuss this roadmap: [Link to issue](https://github.com/strai-project/strai/issues/31966)

## Other Images
* [`straiproject/strai-ml`](https://hub.docker.com/repository/docker/straiproject/strai-ml) - This image with common ML libraries to make development & deployment more smooth!
