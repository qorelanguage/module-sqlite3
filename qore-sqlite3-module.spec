%define module_api %(qore --module-api 2>/dev/null)
%define module_dir %{_libdir}/qore-modules

%if 0%{?sles_version}

%define dist .sles%{?sles_version}

%else
%if 0%{?suse_version}

# get *suse release major version
%define os_maj %(echo %suse_version|rev|cut -b3-|rev)
# get *suse release minor version without trailing zeros
%define os_min %(echo %suse_version|rev|cut -b-2|rev|sed s/0*$//)

%if %suse_version > 1010
%define dist .opensuse%{os_maj}_%{os_min}
%else
%define dist .suse%{os_maj}_%{os_min}
%endif

%endif
%endif

# see if we can determine the distribution type
%if 0%{!?dist:1}
%define rh_dist %(if [ -f /etc/redhat-release ];then cat /etc/redhat-release|sed "s/[^0-9.]*//"|cut -f1 -d.;fi)
%if 0%{?rh_dist}
%define dist .rhel%{rh_dist}
%else
%define dist .unknown
%endif
%endif

Summary: Sqlite3 DBI module for Qore
Name: qore-sqlite3-module
Version: 1.0.1
Release: 1%{dist}
License: LGPL
Group: Development/Languages
URL: http://www.qore.org
Source: http://prdownloads.sourceforge.net/qore/%{name}-%{version}.tar.gz
#Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: /usr/bin/env
Requires: qore-module-api-%{module_api}
BuildRequires: gcc-c++
BuildRequires: qore-devel
BuildRequires: qore
BuildRequires: openssl-devel
# Sqlite RPM package name are different in distros
# fc
%if 0%{?fedora_version}
Requires: sqlite > 3.0
BuildRequires: sqlite-devel > 3.0
%else
#%if 0%{?suse_version}
Requires: sqlite3
BuildRequires: sqlite3-devel
#%endif
%endif

%description
Sqlite3 DBI driver module for the Qore Programming Language.


%if 0%{?suse_version}
%debug_package
%endif

%prep
%setup -q
%ifarch x86_64 ppc64 x390x
c64=--enable-64bit
%endif
./configure RPM_OPT_FLAGS="$RPM_OPT_FLAGS" --prefix=/usr --disable-debug $c64

%build
%{__make}

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{module_dir}
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/qore-pgsql-module
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{module_dir}
%doc COPYING README RELEASE-NOTES ChangeLog AUTHORS test/sqlite3test-basic.q test/sqlite3test-threading.q test/blob.png docs/sqlite3-module-doc.html

%changelog
* Fri Apr 16 2010 Petr Vanek <petr.vanek@qoretechnologies.com>
- updated to version 1.0.1

* Fri Jun 5 2009 Petr Vanek <petr.vanek@qoretechnologies.com>
- initial spec file for separate sqlite3 release
