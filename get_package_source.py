#!/usr/bin/python
from optparse import OptionParser
import re
import urlparse
from launchpadlib.launchpad import Launchpad
from subprocess import call
import sys
import os
import glob
import shutil

def create_webroot_url_from_self_link(self_link):
    scheme, netloc, _, _, _ = urlparse.urlsplit(self_link)
    netloc = netloc.lstrip("api.")
    return u"%s://%s/" % (scheme, netloc)


def get_dsc(archive, package, series):
    re_version = re.compile(r"^\d+\:")
    x = archive.getPublishedSources(exact_match=True, source_name=package,
                                    distro_series=series)
    webroot = create_webroot_url_from_self_link(archive.self_link)
    version = x[0].source_package_version
    version = re_version.sub("", version, 1)
    if archive.owner.name == "ubuntu-archive":
        yield "%subuntu/+archive/primary/+files/%s_%s.dsc" \
            % (webroot, x[0].source_package_name, version)
    else:
        yield "%s/~ci-train-ppa-service/+archive/ubuntu/stable-phone-overlay/+files/%s_%s.dsc" \
            % (webroot, x[0].source_package_name, version)


def main():
    parser = OptionParser(usage="usage: %prog [options] pkgname[:dirname] ...")
    parser.add_option(
        "-l", "--launchpad", dest="launchpad_instance", default="production")
    parser.add_option(
        "-c", "--cwd", dest="cwd", default=None)
    options, args = parser.parse_args()
    launchpad = Launchpad.login_anonymously('just testing', options.launchpad_instance)
    ubuntu = launchpad.distributions["ubuntu"]
    distro_archive = ubuntu.main_archive
    overlay_archive = launchpad.people["ci-train-ppa-service"].\
        getPPAByName(name="stable-phone-overlay")
    series = ubuntu.getSeries(name_or_version="xenial")
    old_wd = None

    if options.cwd is not None:
        os.chdir(os.path.abspath(options.cwd))

    for pkg_desc in args:
        re_version = re.compile(r"^\d+\:")

        names = pkg_desc.split(":")
        source_package = names[0]
        dir_name       = source_package

        if len(names) > 1:
            dir_name = names[1]

        if os.path.exists(dir_name):
            shutil.rmtree(dir_name)

        #first try the overlay archive
        archive = overlay_archive
        x = overlay_archive.getPublishedSources(exact_match=True,
                                                source_name=source_package,
                                                distro_series=series)
        if len(x) == 0:
            archive = distro_archive
            x = distro_archive.getPublishedSources(exact_match=True,
                                                   source_name=source_package,
                                                   distro_series=series)
            if len(x) == 0:
                print("Could not find "+source_package)
                sys.exit(1)
            else:
                print(source_package+" found in Archive")
        else:
            print(source_package+" found in overlay ppa")

        x = x[0].source_package_version

        generator = get_dsc(archive, source_package, series)
        for i in generator:
            call(["dget", i, "--quiet", "--download-only"])
            for dsc_file in glob.glob('%s*dsc' % source_package):
                call(["dpkg-source", "-x", dsc_file])
            for junk_files in glob.glob("*.?z"):
                os.remove(junk_files)
            for junk_files in glob.glob("*.dsc"):
                os.remove(junk_files)
            for fetched_module_directory in glob.glob('%s*' % source_package):
                os.rename(fetched_module_directory, dir_name)

        if old_wd is not None:
            os.chdir(os.path.abspath(old_wd))
main()
