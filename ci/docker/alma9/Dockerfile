FROM gitlab-registry.cern.ch/linuxsupport/alma9-base:latest

ADD trigger /trigger
RUN dnf install -y epel-release &&\
    dnf update -y &&\
    dnf install -y cmake ninja-build g++ cppcheck nano git git-lfs python-pip &&\
    dnf clean all
RUN pip install pre-commit clang-format

RUN echo Triggered at: $(cat /trigger) >> /ISSUE &&\
    echo "*******************" >> /ISSUE &&\
    g++ --version &> /ISSUE &&\
    echo "*******************" >> /ISSUE &&\
    echo Ninja Version: $(ninja-build --version) >> /ISSUE &&\
    echo "*******************" >> /ISSUE &&\
    cmake --version &> /ISSUE &&\
    echo "*******************" >> /ISSUE &&\
    uname -a &> /ISSUE
