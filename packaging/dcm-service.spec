Name:       dcm-service
Summary:    A media DCM(Digital Contents Management) Service
Version:    0.0.6
Release:    0
Group:      Multimedia/Service
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001:     %{name}.manifest

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gthread-2.0)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(sqlite3)
BuildRequires:  pkgconfig(capi-media-image-util)
BuildRequires:  pkgconfig(capi-media-vision)
BuildRequires:  pkgconfig(media-thumbnail)
BuildRequires:  pkgconfig(libmedia-utils)
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(mmutil-imgp)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(uuid)

%description
This package provides a media DCM(Digital Contents Management) service

%package devel
Summary:    A media DCM(Digital Contents Management) service(Development)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
This package provides a media DCM(Digital Contents Management) service(Development files included)

%prep
%setup -q
cp %{SOURCE1001} .

%build
%cmake .
make %{?jobs:-j%jobs}
#export CFLAGS+=" -Wextra -Wno-array-bounds"
#export CFLAGS+=" -Wno-ignored-qualifiers -Wno-unused-parameter -Wshadow"
#export CFLAGS+=" -Wwrite-strings -Wswitch-default"

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/%{_datadir}/license
cp -rf %{_builddir}/%{name}-%{version}/LICENSE.APLv2.0 %{buildroot}/%{_datadir}/license/%{name}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/*.so*
%{_bindir}/*
%{_datadir}/license/%{name}

%files devel
%manifest %{name}.manifest
%{_libdir}/*.so
%{_libdir}/pkgconfig/%{name}.pc
