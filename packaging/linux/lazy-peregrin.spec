Name:           lazy-peregrin
Version:        %{version}
Release:        1%{?dist}
Summary:        The Lazy Peregrin, a screensaver from "Anathem"
License:        MIT
Source0:        lazy-peregrin-%{version}.tar.gz
BuildRequires:  gcc, make, pkgconfig(sdl2)
Requires:       swayidle

%description
The Lazy Peregrin is the problem of a wandering fraa in Neal Stephenson's novel
Anathem who must call at a number of maths and wants to walk as little as he
can: the travelling salesman problem.

This screensaver draws an island of maths, shows a syntactic device trying the
routes one by one and Saunt Grod's Machine holding them all at once, and then
sends a small fraa toddling round the shortest way.

On Wayland desktops it starts by itself after five minutes idle.
lazy-peregrin-screensaver changes the delay or turns it off.

%prep
%setup -q

%build
make %{?_smp_mflags} CFLAGS="%{optflags}" LDFLAGS="%{?__global_ldflags}"

%install
make install DESTDIR=%{buildroot} PREFIX=%{_prefix} SYSCONFDIR=%{_sysconfdir}
rm -rf %{buildroot}%{_datadir}/doc/lazy-peregrin

%files
%license LICENSE
%doc README.md
%{_bindir}/lazy-peregrin
%{_bindir}/lazy-peregrin-screensaver
%{_datadir}/applications/lazy-peregrin.desktop
%{_datadir}/icons/hicolor/256x256/apps/lazy-peregrin.png
%config(noreplace) %{_sysconfdir}/xdg/autostart/lazy-peregrin-screensaver.desktop
