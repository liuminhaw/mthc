# Maintainer: Min-Haw Liu <lmhaw@hey.com>
pkgname=mthc
pkgver=1.0.0
pkgrel=1
pkgdesc="Markdown → HTML converter written in C"
arch=('x86_64')
url='https://mthc.lmhaw.dev'  
license=('MIT')                 
depends=('libunistring' 'pcre2')
makedepends=('gcc' 'make')
checkdepends=('valgrind')

source=("${pkgname}-${pkgver}.tar.gz")
sha256sums=('SKIP')  # run `updpkgsums` after creating the tarball

build() {
  cd "${srcdir}/${pkgname}-${pkgver}"
  make
}

check() {
  cd "${srcdir}/${pkgname}-${pkgver}"
  make check
  make mem-check
}

package() {
  cd "${srcdir}/${pkgname}-${pkgver}"
  make DESTDIR="${pkgdir}" PREFIX=/usr install

  # Docs
  install -Dm644 README.md "${pkgdir}/usr/share/doc/${pkgname}/README.md"
  if [[ -d docs ]]; then
    mkdir -p "${pkgdir}/usr/share/doc/${pkgname}"
    cp -r --preserve=timestamps docs "${pkgdir}/usr/share/doc/${pkgname}/"
  fi

  # License
  if [[ -f LICENSE ]]; then
    install -Dm644 LICENSE "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
  fi
}
