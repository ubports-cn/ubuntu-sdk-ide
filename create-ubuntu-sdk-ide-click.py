#!/usr/bin/python
from optparse import OptionParser
import re
import urlparse
from launchpadlib.launchpad import Launchpad
from subprocess import call
import subprocess 
import sys
import os
import glob
import shutil
import requests

ide_packages = [
                "android-tools-adb",
                "android-tools-fastboot",
                "apparmor-easyprof",
                "apparmor-easyprof-ubuntu",
                "binutils",
                "bzr",
                "click",
                "click-apparmor",
                "click-dev",
                "click-doc",
                "click-reviewers-tools",
                "dpkg-dev",
                "fontconfig",
                "fontconfig-config",
                "fonts-dejavu-core",
                "freetds-common",
                "gdb",
                "gdb-multiarch",
                "gir1.2-click-0.4",
                "gir1.2-gee-0.8",
                "gir1.2-json-1.0",
                "libasound2",
                "libasound2-data",
                "libasound2-plugins",
                "libasyncns0",
                "libatk1.0-0",
                "libatk1.0-data",
                "libavahi-client3",
                "libavahi-common-data",
                "libavahi-common3",
                "libbabeltrace-ctf1",
                "libbabeltrace1",
                "libbotan-1.10-1",
                "libcairo2",
                "libclick-0.4-0",
                "libcups2",
                "libdatrie1",
                "libdpkg-perl",
                "libdrm-amdgpu1",
                "libdrm-intel1",
                "libdrm-nouveau2",
                "libdrm-radeon1",
                "libegl1-mesa",
                "libepoxy0",
                "libfcitx-config4",
                "libfcitx-utils0",
                "libfftw3-single3",
                "libflac8",
                "libfontconfig1",
                "libfontenc1",
                "libfreetype6",
                "libgbm1",
                "libgdk-pixbuf2.0-0",
                "libgdk-pixbuf2.0-common",
                "libgee-0.8-2",
                "libgl1-mesa-dri",
                "libgl1-mesa-glx",
                "libglapi-mesa",
                "libgomp1",
                "libgraphite2-3",
                "libgstreamer-plugins-base1.0-0",
                "libgstreamer1.0-0",
                "libgtk2.0-0",
                "libgtk2.0-common",
                "libharfbuzz0b",
                "libice6",
                "libjack-jackd2-0",
                "libjbig0",
                "libjpeg-turbo8",
                "libjpeg8",
                "libjs-jquery",
                "libjs-sphinxdoc",
                "libjs-underscore",
                "libjson-glib-1.0-0",
                "libjson-glib-1.0-common",
                "libllvm3.8",
                "libltdl7",
                "libmtdev1",
                "libmysqlclient20",
                "libnih-dbus1",
                "libodbc1",
                "libogg0",
                "liborc-0.4-0",
                "libpango-1.0-0",
                "libpangocairo-1.0-0",
                "libpangoft2-1.0-0",
                "libpciaccess0",
                "libpcre16-3",
                "libpixman-1-0",
                "libpq5",
                "libpulse0",
                "libpulsedsp",
                "libpython-stdlib",
                "libpython2.7-minimal",
                "libpython2.7-stdlib",
                "libsamplerate0",
                "libsm6",
                "libsndfile1",
                "libspeexdsp1",
                "libsybdb5",
                "libtdb1",
                "libthai-data",
                "libthai0",
                "libtiff5",
                "libvorbis0a",
                "libvorbisenc2",
                "libwayland-client0",
                "libwayland-server0",
                "libwebrtc-audio-processing-0",
                "libx11-xcb1",
                "libxaw7",
                "libxcb-dri2-0",
                "libxcb-dri3-0",
                "libxcb-glx0",
                "libxcb-icccm4",
                "libxcb-image0",
                "libxcb-keysyms1",
                "libxcb-present0",
                "libxcb-randr0",
                "libxcb-render-util0",
                "libxcb-render0",
                "libxcb-shape0",
                "libxcb-shm0",
                "libxcb-sync1",
                "libxcb-util1",
                "libxcb-xf86dri0",
                "libxcb-xfixes0",
                "libxcb-xinerama0",
                "libxcb-xkb1",
                "libxcb-xv0",
                "libxcomposite1",
                "libxcursor1",
                "libxdamage1",
                "libxfixes3",
                "libxfont1",
                "libxi6",
                "libxinerama1",
                "libxkbcommon-x11-0",
                "libxkbcommon0",
                "libxkbfile1",
                "libxmu6",
                "libxpm4",
                "libxrandr2",
                "libxrender1",
                "libxshmfence1",
                "libxslt1.1",
                "libxt6",
                "libxtst6",
                "libxxf86vm1",
                "make",
                "mysql-common",
                "phablet-tools",
                "pulseaudio",
                "pulseaudio-utils",
                "python",
                "python-bzrlib",
                "python-chardet",
                "python-configobj",
                "python-dbus",
                "python-httplib2",
                "python-kerberos",
                "python-keyring",
                "python-launchpadlib",
                "python-lazr.restfulclient",
                "python-lazr.uri",
                "python-lzma",
                "python-minimal",
                "python-oauth",
                "python-pkg-resources",
                "python-requests",
                "python-simplejson",
                "python-six",
                "python-urllib3",
                "python-wadllib",
                "python-xdg",
                "python-zope.interface",
                "python2.7",
                "python2.7-minimal",
                "python3-apparmor",
                "python3-apparmor-click",
                "python3-click-package",
                "python3-libapparmor",
                "python3-lxml",
                "python3-magic",
                "python3-simplejson",
                "python3-xdg",
                "repo",
                "ubuntu-sdk-ide",
                "ubuntu-sdk-tools",
                "x11-common",
                "x11-xkb-utils",
                "xserver-common",
                "xserver-xephyr",

]


def create_webroot_url_from_self_link(self_link):
    scheme, netloc, _, _, _ = urlparse.urlsplit(self_link)
    netloc = netloc.lstrip("api.")
    return u"%s://%s/" % (scheme, netloc)


def get_deb(archive, package, series, arch):
    re_version = re.compile(r"^\d+\:")
    arch_series = series.getDistroArchSeries(archtag=arch)
    x = archive.getPublishedBinaries(binary_name=package,
                                     distro_arch_series=arch_series,
                                     status="Published", pocket='Release',
                                     exact_match=True)

    webroot = create_webroot_url_from_self_link(archive.self_link)
    version = x[0].binary_package_version
    version = re_version.sub("", version, 1)
    if archive.owner.name == "ubuntu-archive":
        yield "%subuntu/+archive/primary/+files/%s_%s_%s.deb" \
            % (webroot, x[0].binary_package_name, version, arch)
    else:
        yield "%s/~ubuntu-sdk-team/+archive/ubuntu/ppa/+files/%s_%s_%s.deb" \
            % (webroot, x[0].binary_package_name, version, arch)


def main():
    parser = OptionParser(usage="usage: %prog [options] source ...")
    parser.add_option(
        "-l", "--launchpad", dest="launchpad_instance", default="production")
    parser.add_option(
        "-a", "--arch", dest="arch", default="armhf", choices=['armhf', 'i386', 'amd64',], help="architecture of the package")

    options, args = parser.parse_args()
    arch = options.arch
    launchpad = Launchpad.login_anonymously('just testing',
                                            options.launchpad_instance)
    ubuntu = launchpad.distributions["ubuntu"]
    distro_archive = ubuntu.main_archive
    overlay_archive = launchpad.people["ubuntu-sdk-team"].\
        getPPAByName(name="ppa")
    series = ubuntu.getSeries(name_or_version="xenial")

    arch_series = series.getDistroArchSeries(archtag=arch)

    build_directory = "./click-%s/" % arch
    if not os.path.exists(build_directory):
        os.makedirs(build_directory)
    src="./click/"
    src_files = os.listdir(src)
    for file_name in src_files:
        full_file_name = os.path.join(src, file_name)
        if (os.path.isfile(full_file_name)):
            shutil.copy(full_file_name, build_directory)

    for ide_package in ide_packages:
        re_version = re.compile(r"^\d+\:")
        archive = overlay_archive
        x = overlay_archive.getPublishedBinaries(
            binary_name=ide_package,
            distro_arch_series=arch_series,
            status="Published", pocket='Release',
            exact_match=True)

        if len(x) == 0:
            archive = distro_archive
            x = distro_archive.getPublishedBinaries(
                binary_name=ide_package,
                distro_arch_series=arch_series,
                status="Published", pocket='Release',
                exact_match=True)
            if len(x) == 0:
                print("Could not find "+ide_package)
                sys.exit(1)
            else:
                print(ide_package+" found in Archive")
        else:
            print(ide_package+" found in SDK PPA")

        x = x[0].binary_package_version

        generator = get_deb(archive, ide_package, series, arch)
        for i in generator:
            request = requests.get(i)
            if request.status_code != 200:
                i = re.sub('_%s.deb' % arch, '_all.deb', i)
               
            if ide_package != 'ubuntu-ui-toolkit':
                binary_package_name = re.sub(r'-opensource-src',
                                             r'',
                                             ide_package.rstrip())
            else:
                binary_package_name = re.sub(r'ubuntu-ui-toolkit',
                                             r'ubuntu-sdk',
                                             ide_package.rstrip())
            call(["wget", i, "-nv"])
            for deb_file in glob.glob('%s*deb' % ide_package):
                call(["dpkg-deb", "-x", deb_file, build_directory])
            for junk_files in glob.glob("*.deb"):
                os.remove(junk_files)
#    click_package_rev = subprocess.Popen(["bzr", "revno"], stdout=subprocess.PIPE).communicate()[0]
#    click_package_rev = click_package_rev.rstrip()
#    manifest_file = open('click/manifest.json','r')
#    manifest_with_orig_version = manifest_file.read()
#    manifest_file.close()
#    manifest_with_new_version = re.sub("\"version\":\s+\".*\"", "\"version\": \"%s\"" % click_package_rev, manifest_with_orig_version)
#    manifest_file = open('click/manifest.json','w')
#    manifest_file.write(manifest_with_new_version)
#    manifest_file.close()



    call(["create_kit.sh","-a",arch])
    s=open("%s/manifest.json" % build_directory).read()
    s=s.replace('armhf', '%s' % arch)
    f=open("%s/manifest.json" % build_directory, 'w')
    f.write(s)
    f.close()
    call(["click", "build", build_directory])
main()
