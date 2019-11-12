#
# python-bareos spec file.
# It works with SUSE and Redhat flavors.
#

#
# REDHAT:
#
# Redhat builds requires some preparations:
#
# It requires additional RPM macro definitions,
# that are default on SUSE.
# Copy the provided .rpmmacros into the home directory of your build user:
#
# cp .rpmmacros ~
#
#
# The macro _specfile must point to the file location of this file.
# Use rpmbuild with following parameters (adapt your path accordingly):
#
# rpmbuild -ba -D "_specfile  %_specdir/python-bareos.spec" ~/rpmbuild/SPECS/python-bareos.spec
#


%if 0%{?rhel} >= 0 || 0%{?fedora} >= 0
%define debug_package %{nil}
%endif

%if 0%{?rhel} >= 8 || 0%{?fedora} >= 32
%define skip_python2 1
%endif

%{?!python_module:%define python_module() python-%{**} python3-%{**}}
Name:           python-bareos
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
BuildRequires:  %{python_module devel}
BuildRequires:  %{python_module setuptools}
#BuildRequires:  fdupes

%python_subpackages

%description
Bareos - Backup Archiving Recovery Open Sourced - Python module

This packages contains a python module to interact with a Bareos backup system.
It also includes some tools based on this module.

%prep
#%%setup -q -n %{name}
%setup -q

%build
%python_build

%install
%python_install
#%%python_expand %%fdupes %%{buildroot}%%{$python_sitelib}

%check
# does not work, as it tries to download other packages from pip
#%%python_exec setup.py test

%files %{python_files}
%defattr(-,root,root,-)
%doc README.rst
%python3_only %{_bindir}/*
%{python_sitelib}/*

%changelog
