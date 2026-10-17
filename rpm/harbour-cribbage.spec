Name:       harbour-cribbage

Summary:    Cribbage against a bot
Version:    0.1.0
Release:    1
License:    GPLv3
URL:        https://github.com/klahr/cribbage
Source0:    %{name}-%{version}.tar.bz2
Requires:   sailfishsilica-qt5 >= 0.10.9
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  desktop-file-utils

%description
Two-player cribbage against a bot in three levels. The play, the hands
and the crib are counted for you, one score at a time on the cards that
made it.


%prep
%setup -q -n %{name}-%{version}

%build

%qmake5 

%make_build

%check
make check

%install
%qmake5_install


desktop-file-install --delete-original         --dir %{buildroot}%{_datadir}/applications                %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
