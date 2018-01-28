#!/bin/bash
set -ev
which shellcheck > /dev/null && shellcheck "$0" # Run shellcheck on this if available

git clone --depth 1 https://github.com/randombit/botan-ci-tools

if [ "$TRAVIS_OS_NAME" = "linux" ]; then

    if [ "$BUILD_MODE" = "coverage" ]; then

        # SoftHSMv1 in 14.04 does not work
        # Installs prebuilt SoftHSMv2 binaries into /tmp
        tar -C / -xvjf botan-ci-tools/softhsm2-trusty-bin.tar.bz2
        /tmp/softhsm/bin/softhsm2-util --init-token --free --label test --pin 123456 --so-pin 12345678

        pip install --user coverage
        pip install --user codecov==2.0.10

    elif [ "$BUILD_MODE" = "sonar" ]; then

        tar -C / -xvjf botan-ci-tools/softhsm2-trusty-bin.tar.bz2
        /tmp/softhsm/bin/softhsm2-util --init-token --free --label test --pin 123456 --so-pin 12345678

        wget https://sonarqube.com/static/cpp/build-wrapper-linux-x86.zip
        unzip build-wrapper-linux-x86.zip

    fi

elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
    HOMEBREW_NO_AUTO_UPDATE=1 brew install ccache
fi
