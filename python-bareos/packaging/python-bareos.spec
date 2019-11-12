#
# python-bareos spec file.
#

# based on
# https://docs.fedoraproject.org/en-US/packaging-guidelines/Python_Appendix/
# specifically on
# https://pagure.io/packaging-committee/blob/ae14fdb50cc6665a94bc32f7d984906ce1eece45/f/guidelines/modules/ROOT/pages/Python_Appendix.adoc
#

%define python2_build_requires python2-devel python2-setuptools
%define python3_build_requires python3-devel python3-setuptools

%if 0%{?rhel} > 0 && 0%{?rhel} <= 6
%define skip_python3 1
%define python2_build_requires python-devel python-setuptools
%endif

%global srcname bareos

Name:           python-%{srcname}
Version:        0
Release:        1%{?dist}
License:        AGPL-3.0
Summary:        Backup Archiving REcovery Open Sourced - Python module
Url:            https://github.com/bareos/python-bareos/
Group:          Productivity/Archiving/Backup
Vendor:         The Bareos Team
Source:         %{name}-%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-root
BuildRequires:  python-rpm-macros

BuildArch:      noarch

%global _description %{expand:
Bareos - Backup Archiving Recovery Open Sourced - Python module

This packages contains a python module to interact with a Bareos backup system.
It also includes some tools based on this module.}

%description %_description

%if ! 0%{?skip_python2}

%package -n python2-%{srcname}
Summary:        %{summary}
BuildRequires:  %{python2_build_requires}
%{?python_provide:%python_provide python2-%{srcname}}

%description -n python2-%{srcname} %_description

%endif
# skip_python2


%if ! 0%{?skip_python3}

%package -n python3-%{srcname}
Summary:        %{summary}
BuildRequires:  %{python3_build_requires}
%{?python_provide:%python_provide python3-%{srcname}}

%description -n python3-%{srcname} %_description

%endif
# skip_python3

%prep
#%%autosetup -n %%{srcname}-%%{version}
%setup -q

%build
%if ! 0%{?skip_python2}
%py2_build
%endif

%if ! 0%{?skip_python3}
%py3_build
%endif


%install
# Must do the python2 install first because the scripts in /usr/bin are
# overwritten with every setup.py install, and in general we want the
# python3 version to be the default.
%if ! 0%{?skip_python2}
%py2_install
%endif

%if ! 0%{?skip_python3}
%py3_install
%endif

%check
# This does not work,
# as "test" tries to download other packages from pip.
#%%{__python2} setup.py test
#%%{__python3} setup.py test


# Note that there is no %%files section for the unversioned python module if we are building for several python runtimes
%if ! 0%{?skip_python2}
%files -n python2-%{srcname}
%defattr(-,root,root,-)
%doc README.rst
%{python2_sitelib}/%{srcname}/
%{python2_sitelib}/python_%{srcname}-*.egg-info/
# Include binaries only if no python3 package will be build.
%if 0%{?skip_python3}
%{_bindir}/*
%endif
%endif
# skip_python2


%if ! 0%{?skip_python3}
%files -n python3-%{srcname}
%defattr(-,root,root,-)
%doc README.rst
%{python3_sitelib}/%{srcname}/
%{python3_sitelib}/python_%{srcname}-*.egg-info/
%{_bindir}/*
%endif
# skip_python3


%changelog
