Name:           mthc
Version:        1.0.3
Release:        1%{?dist}
Summary:        Markdown to HTML converter written in C
License:        MIT
URL:            https://mthc.lmhaw.dev
Source0:        %{name}-%{version}.tar.gz

# Build-time deps (adjust for your distro)
BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  pkgconfig
BuildRequires:  libunistring-devel
BuildRequires:  pcre2-devel

%description
mthc is a converter that generate HTML from Markdown. This package contains the mthc CLI.

%prep
%autosetup

%build
%set_build_flags
# Ensure the final program is named 'mthc'; adjust if you prefer another name
%make_build BINARY=%{name}

%install
# Install into the buildroot using your Makefile's DESTDIR
%make_install BINARY=%{name} PREFIX=%{_prefix}

%check
# Fail the build if tests or mem-check fail (non-zero exit)
make check
make mem-check

%files
%license LICENSE
%doc docs/manual.md
%{_bindir}/%{name}

%changelog
* Sun Jan 04 2026 Min-Haw Liu <liuminhaw@gmail.com> - 1.0.3-1
- Remember theme settings within the site using localStorage
- Prevent parsing content in html tag block
* Sat Sep 13 2025 Min-Haw Liu <liuminhaw@gmail.com> - 1.0.2-1
- Tweak default image styling by removing bottom margin
* Sat Sep 13 2025 Min-Haw Liu <liuminhaw@gmail.com> - 1.0.1-1
- Fix generated HTML default image styling
* Thu Sep 05 2025 Min-Haw Liu <liuminhaw@gmail.com> - 1.0.0-1
- Initial package

