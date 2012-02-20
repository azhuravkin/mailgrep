Name: mailgrep
Version: 12
Release: 1.NSYS
Group: Applications/Text
Summary: Application for easy discovering sendmail and exim logs
License: GPL
Source0: %{name}-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-root
BuildRequires: pcre pcre-devel
Requires: pcre

%description

%prep
%setup

%install
make clean all
mkdir -p $RPM_BUILD_ROOT/usr/bin
install mailgrep $RPM_BUILD_ROOT/usr/bin

%files
/usr/bin/mailgrep

%clean
[ $RPM_BUILD_ROOT != "/" ] && rm -rf $RPM_BUILD_ROOT

%changelog
* Tue Feb 08 2011 Juravkin Alexander <rinus@nsys.by>
- Add support .gz and .bz2 archives

* Sat Aug 15 2009 Juravkin Alexander <rinus@nsys.by>
- Add color selection pattern.

* Thu May 07 2009 Juravkin Alexander <rinus@nsys.by>
- Add support postfix log format.
- Add support 'nocolor' option.

* Sat May 02 2009 Juravkin Alexander <rinus@nsys.by>
- Add support exim log format.
- Add support debug mode.

* Thu Feb 12 2009 Juravkin Alexander <rinus@nsys.by>
- Fix bug.
- Add dynamic width of the separator.
- Show a separator at the beginning and end.

* Thu Jul 31 2008 Juravkin Alexander <rinus@nsys.by>
- Build
