# Maintainer: Benjamin Bolton <benjamin@bennybolton.com>

_pkgname=bstatus
pkgname=$_pkgname-git
pkgver=0.1.0.r0.g4ebaaf6
pkgrel=1
pkgdesc="Simple and extensible status line generator."
arch=('i686' 'x86_64')
url="https://github.com/BennyBolton/bstatus"
license=("GPL")
depends=(python)
makedepends=(git)
provides=("bstatus")
source=("git://github.com/BennyBolton/bstatus.git")
md5sums=("SKIP")

pkgver() {
  cd "$srcdir/$_pkgname"
  git describe --long | sed -r 's/([^-]*-g)/r\1/;s/-/./g'
}

build() {
  cd "$srcdir/$_pkgname"
  make
}

package() {
  cd "$srcdir/$_pkgname"
  make DESTDIR="$pkgdir" install
}
