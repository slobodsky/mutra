# -*- mode: shell-script; -*-
# Maintainer: Nick Slobodsky <software@slobodsky.ru>
# \todo move this to appropriate subdir. Provide correct dependencies. Setup usual model with unpacking tar sources.
pkgname=('mutra' 'libmutra' 'libmutra-dev')
pkgbase="mutra"
pkgver=0.0.1
pkgrel=0.001
pkgdesc="The Music Trainer application & libraries"
arch=('x86_64')
url="https://software.slobodsky.ru/mutra"
license=('GPL')
# groups=()
depends=()
makedepends=('cmake')
checkdepends=()
optdepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=()
install=
changelog=
source=("$pkgbase-$pkgver.tar.gz")
noextract=()
md5sums=()
validpgpkeys=()

# prepare() {
#	cd "$pkgname-$pkgver"
#	patch -p1 -i "$srcdir/$pkgname-$pkgver.patch"
# }

build() {
#	cd "$pkgbase-$pkgver"
	cmake -B build -S "$pkgbase-$pkgver" -DCMAKE_INSTALL_PREFIX=/usr
	cmake --build build
}

check() {
	cd "$pkgname-$pkgver"
#	make -k check
}

package_libmutra() {
	# options and directives that can be overridden
	pkgdesc="Music Trainer shared libraries"
	# arch=()
	# url=""
	# license=()
	# groups=()
	# depends=()
	# optdepends=()
	# provides=()
	# conflicts=()
	# replaces=()
	# backup=()
	# options=()
	# install=
	# changelog=

#	cd "$pkgbase-$pkgver"
	cmake --install build --prefix "$pkgdir/usr" --component libmutra
}

package_mutra() {
	# options and directives overrides
	pkgdesc="Music Trainer GUI application"

#	cd "$pkgbase-$pkgver"
	cmake --install build --prefix "$pkgdir/usr" --component mutra
}

package_libmutra-dev() {
	# options and directives overrides
	pkgdesc="Music Trainer development files"

#	cd "$pkgbase-$pkgver"
	cmake --install build --prefix "$pkgdir/usr" --component libmutra-dev
}

postinstall() {
    update-mime-database /usr/share/mime
    update-desktop-database
}
