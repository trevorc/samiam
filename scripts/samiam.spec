Summary: A text-mode sam interpreter.
Name: samiam
Version: 1.1.0
Release: 1
License: MIT
Group: Development/Languages
Source0: samiam-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-buildroot

%description
This package provides a simple, general purpose text-mode frontend for
the sam interpreter library libsam.

%prep
%setup -n samiam

%build
scons RPM_OPT_FLAGS="$RPM_OPT_FLAGS"
make -C doc

%install
rm -Rf $RPM_BUILD_ROOT
scons install DESTDIR=$RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -Rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc COPYING CHANGELOG TODO-linking README TODO

/usr/bin/samiam
#/usr/man/man1/samiam.1
/usr/lib/libsam.so.%{version}

%changelog
* Tue May 22 2007 Trevor Caira <trevor@caira.com>
- Initial release
