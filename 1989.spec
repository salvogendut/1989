Name:           1989
Version:        0.1.0
Release:        1%{?dist}
Summary:        NeXT (Motorola 68K) emulator

License:        GPL-2.0-or-later
URL:            https://github.com/salvogendut/1989
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  libtool
BuildRequires:  pkgconfig(sdl3)
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib

%description
1989 is an SDL3-based emulator of the NeXT family of Motorola 68K
workstations (NeXT Computer, NeXTcube, NeXTcube Turbo, NeXTstation and
NeXTstation Color, plus the optional NeXTdimension graphics board). It is
built on the Previous NeXT emulator code base: the m68k core comes from
WinUAE and the i860 NeXTdimension emulation from Jason Eckhardt. The NeXT
firmware images are distributed with the project.

%prep
%autosetup

%build
autoreconf -fiv
%configure
%make_build

%install
%make_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/io.github.salvogendut.Emulator1989.desktop
appstream-util validate-relax --nonet %{buildroot}%{_datadir}/metainfo/io.github.salvogendut.Emulator1989.metainfo.xml

%files
%license LICENSE
%doc README.md ROMS.md INSTALL.md USAGE.md CONTROLS.md
%{_bindir}/%{name}
%{_bindir}/ditool
%{_mandir}/man1/%{name}.1*
%{_datadir}/applications/io.github.salvogendut.Emulator1989.desktop
%{_datadir}/metainfo/io.github.salvogendut.Emulator1989.metainfo.xml
%{_datadir}/icons/hicolor/*/apps/io.github.salvogendut.Emulator1989.png
%dir %{_datadir}/%{name}
%dir %{_datadir}/%{name}/roms
%{_datadir}/%{name}/roms/*.BIN
%{_datadir}/%{name}/roms/README
%dir %{_datadir}/%{name}/disks
%{_datadir}/%{name}/disks/*.zip

%changelog
* Fri Oct 02 2026 Salvatore Bognanni <salvogendut@gmail.com> - 0.1.0-1
- Initial scaffolding and integration of the Previous 4.3 NeXT emulator
  into the shared SDL3 "happy years" build conventions.