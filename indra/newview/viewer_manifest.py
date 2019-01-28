#!/usr/bin/env python
"""\
@file viewer_manifest.py
@author Ryan Williams
@brief Description of all installer viewer files, and methods for packaging
       them into installers for all supported platforms.

$LicenseInfo:firstyear=2006&license=viewergpl$
Second Life Viewer Source Code
Copyright (c) 2006-2009, Linden Research, Inc.

The source code in this file ("Source Code") is provided by Linden Lab
to you under the terms of the GNU General Public License, version 2.0
("GPL"), unless you have obtained a separate licensing agreement
("Other License"), formally executed by you and Linden Lab.  Terms of
the GPL can be found in doc/GPL-license.txt in this distribution, or
online at http://secondlifegrid.net/programs/open_source/licensing/gplv2

There are special exceptions to the terms and conditions of the GPL as
it is applied to this Source Code. View the full text of the exception
in the file doc/FLOSS-exception.txt in this software distribution, or
online at
http://secondlifegrid.net/programs/open_source/licensing/flossexception

By copying, modifying or distributing this software, you acknowledge
that you have read and understood your obligations described above,
and agree to abide by those obligations.

ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
COMPLETENESS OR PERFORMANCE.
$/LicenseInfo$
"""
import sys
import os
import os.path
import shutil
import errno
import json
import re
import tarfile
import time
import random
import subprocess

viewer_dir = os.path.dirname(__file__)
# Add indra/lib/python to our path so we don't have to muck with PYTHONPATH.
# Put it FIRST because some of our build hosts have an ancient install of
# indra.util.llmanifest under their system Python!
sys.path.insert(0, os.path.join(viewer_dir, os.pardir, "lib", "python"))
from indra.util.llmanifest import LLManifest, main, proper_windows_path, path_ancestors, CHANNEL_VENDOR_BASE, RELEASE_CHANNEL, ManifestError
try:
    from llbase import llsd
except ImportError:
    from indra.base import llsd

class ViewerManifest(LLManifest):
    def is_packaging_viewer(self):
        # Some commands, files will only be included
        # if we are packaging the viewer on windows.
        # This manifest is also used to copy
        # files during the build (see copy_w_viewer_manifest
        # and copy_l_viewer_manifest targets)
        return 'package' in self.args['actions']

    def package_skin(self, xml, skin_dir):
        self.path(xml)
        self.path(skin_dir + "/*")
        # include the entire textures directory recursively
        if self.prefix(src=skin_dir+"/textures"):
            self.path("*/*.tga")
            self.path("*/*.j2c")
            self.path("*/*.jpg")
            self.path("*/*.png")
            self.path("*.tga")
            self.path("*.j2c")
            self.path("*.jpg")
            self.path("*.png")
            self.path("textures.xml")
            self.end_prefix(skin_dir+"/textures")

    def construct(self):
        super(ViewerManifest, self).construct()
        self.path(src="../../scripts/messages/message_template.msg", dst="app_settings/message_template.msg")
        self.path(src="../../etc/message.xml", dst="app_settings/message.xml")

        if self.is_packaging_viewer():
            if self.prefix(src="app_settings"):
                self.exclude("logcontrol.xml")
                self.exclude("logcontrol-dev.xml")
                self.path("*.pem")
                self.path("*.ini")
                self.path("*.xml")
                self.path("*.db2")

                # include the entire shaders directory recursively
                self.path("shaders")

                # ... and the entire windlight directory
                self.path("windlight")

                # ... and the included spell checking dictionaries
                self.path("dictionaries")
                self.path("ca-bundle.crt")

                # include the extracted packages information (see BuildPackagesInfo.cmake)
                self.path(src=os.path.join(self.args['build'],"packages-info.txt"), dst="packages-info.txt")


                self.end_prefix("app_settings")

            if self.prefix(src="character"):
                self.path("*.llm")
                self.path("*.xml")
                self.path("*.tga")
                self.end_prefix("character")

            # Include our fonts
            if self.prefix(src="fonts"):
                self.path("*.ttf")
                self.path("*.txt")
                self.end_prefix("fonts")

            # skins
            if self.prefix(src="skins"):
                self.path("paths.xml")
                self.path("default/xui/*/*.xml")
                self.package_skin("Default.xml", "default")
                self.package_skin("dark.xml", "dark")
                self.package_skin("Gemini.xml", "gemini")

                # Local HTML files (e.g. loading screen)
                if self.prefix(src="*/html"):
                    self.path("*.png")
                    self.path("*/*/*.html")
                    self.path("*/*/*.gif")
                    self.end_prefix("*/html")

                self.end_prefix("skins")

            # local_assets dir (for pre-cached textures)
            if self.prefix(src="local_assets"):
                self.path("*.j2c")
                self.path("*.tga")
                self.end_prefix("local_assets")

            # File in the newview/ directory
            self.path("gpu_table.txt")

            #summary.json.  Standard with exception handling is fine.  If we can't open a new file for writing, we have worse problems
            summary_dict = {"Type":"viewer","Version":'.'.join(self.args['version']),"Channel":self.channel_with_pkg_suffix()}
            with open(os.path.join(os.pardir,'summary.json'), 'w') as summary_handle:
                json.dump(summary_dict,summary_handle)

            #we likely no longer need the test, since we will throw an exception above, but belt and suspenders and we get the
            #return code for free.
            if not self.path2basename(os.pardir, "summary.json"):
                print "No summary.json file"

    def standalone(self):
        return self.args['standalone'] == "ON"

    def grid(self):
        return self.args['grid']

    def viewer_branding_id(self):
        return self.args['branding_id']

    def channel(self):
        return self.args['channel']

    def channel_with_pkg_suffix(self):
        fullchannel=self.channel()
        if 'channel_suffix' in self.args and self.args['channel_suffix']:
            fullchannel+=' '+self.args['channel_suffix']
        return fullchannel

    def channel_variant(self):
        global CHANNEL_VENDOR_BASE
        return self.channel().replace(CHANNEL_VENDOR_BASE, "").strip()

    def channel_type(self): # returns 'release', 'beta', 'project', or 'test'
        global CHANNEL_VENDOR_BASE
        channel_qualifier=self.channel().replace(CHANNEL_VENDOR_BASE, "").lower().strip()
        if channel_qualifier.startswith('release'):
            channel_type='release'
        elif channel_qualifier.startswith('beta'):
            channel_type='beta'
        elif channel_qualifier.startswith('alpha'):
            channel_type='alpha'
        elif channel_qualifier.startswith('project'):
            channel_type='project'
        else:
            channel_type='test'
        return channel_type

    def channel_variant_app_suffix(self):
        # get any part of the compiled channel name after the CHANNEL_VENDOR_BASE
        suffix=self.channel_variant()
        # by ancient convention, we don't use Release in the app name
        if self.channel_type() == 'release':
            suffix=suffix.replace('Release', '').strip()
        # for the base release viewer, suffix will now be null - for any other, append what remains
        if len(suffix) > 0:
            suffix = "_"+ ("_".join(suffix.split()))
        # the additional_packages mechanism adds more to the installer name (but not to the app name itself)
        if 'channel_suffix' in self.args and self.args['channel_suffix']:
            suffix+='_'+("_".join(self.args['channel_suffix'].split()))
        return suffix

    def installer_base_name(self):
        global CHANNEL_VENDOR_BASE
        # a standard map of strings for replacing in the templates
        substitution_strings = {
            'channel_vendor_base' : '_'.join(CHANNEL_VENDOR_BASE.split()),
            'channel_variant_underscores':self.channel_variant_app_suffix(),
            'version_underscores' : '_'.join(self.args['version']),
            'arch':self.args['arch']
            }
        return "%(channel_vendor_base)s%(channel_variant_underscores)s_%(version_underscores)s_%(arch)s" % substitution_strings

    def app_name(self):
        global CHANNEL_VENDOR_BASE
        channel_type=self.channel_type()
        if channel_type == 'release':
            app_suffix=''
        else:
            app_suffix=self.channel_variant()
        return CHANNEL_VENDOR_BASE + ' ' + app_suffix

    def app_name_oneword(self):
        return ''.join(self.app_name().split())

    def icon_path(self):
        return "icons/" + ("default", "alpha")[self.channel_type() == "alpha"]

    def extract_names(self,src):
        try:
            contrib_file = open(src,'r')
        except IOError:
            print "Failed to open '%s'" % src
            raise
        lines = contrib_file.readlines()
        contrib_file.close()

        # All lines up to and including the first blank line are the file header; skip them
        lines.reverse() # so that pop will pull from first to last line
        while not re.match("\s*$", lines.pop()) :
            pass # do nothing

        # A line that starts with a non-whitespace character is a name; all others describe contributions, so collect the names
        names = []
        for line in lines :
            if re.match("\S", line) :
                names.append(line.rstrip())
        # It's not fair to always put the same people at the head of the list
        random.shuffle(names)
        return ', '.join(names)

class WindowsManifest(ViewerManifest):
    def is_win64(self):
        return self.args.get('arch') == "x86_64"

    def final_exe(self):
        return self.app_name_oneword()+"Viewer.exe"

    def construct(self):
        super(WindowsManifest, self).construct()

        pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
        relpkgdir = os.path.join(pkgdir, "lib", "release")
        debpkgdir = os.path.join(pkgdir, "lib", "debug")

        if self.is_packaging_viewer():
            # Find singularity-bin.exe in the 'configuration' dir, then rename it to the result of final_exe.
            self.path(src='%s/%s-bin.exe' % (self.args['configuration'],self.viewer_branding_id()), dst=self.final_exe())

        # Plugin host application
        self.path2basename(os.path.join(os.pardir,
                                        'llplugin', 'slplugin', self.args['configuration']),
                           "AlchemyPlugin.exe")

        # Get shared libs from the shared libs staging directory
        if self.prefix(src=os.path.join(os.pardir, 'sharedlibs', self.args['configuration']),
                       dst=""):

            # Get llcommon and deps. If missing assume static linkage and continue.
            try:
                self.path('llcommon.dll')
                self.path('libapr-1.dll')
                self.path('libaprutil-1.dll')
                self.path('libapriconv-1.dll')

            except RuntimeError, err:
                print err.message
                print "Skipping llcommon.dll (assuming llcommon was linked statically)"

            # Mesh 3rd party libs needed for auto LOD and collada reading
            try:
                self.path("glod.dll")
            except RuntimeError, err:
                print err.message
                print "Skipping GLOD library (assumming linked statically)"

            # Get fmodex dll, continue if missing
            try:
                self.path("alut.dll")
                self.path("OpenAL32.dll")
            except:
                print "Skipping openal audio library(assuming other audio engine)"

            # For textures
            if self.args['configuration'].lower() == 'debug':
                self.path("openjpegd.dll")
            else:
                self.path("openjpeg.dll")

            # Vivox runtimes
            self.path("SLVoice.exe")
            self.path("vivoxsdk.dll")
            self.path("ortp.dll")
            self.path("libsndfile-1.dll")
            self.path("vivoxoal.dll")
            self.path("ca-bundle.crt")

            # Hunspell
            self.path("libhunspell.dll")

            # For google-perftools tcmalloc allocator.
            try:
                if self.args['configuration'].lower() == 'debug':
                    self.path('libtcmalloc_minimal-debug.dll')
                else:
                    self.path('libtcmalloc_minimal.dll')
            except:
                print "Skipping libtcmalloc_minimal.dll"

            self.end_prefix()

        self.path(src="licenses-win32.txt", dst="licenses.txt")
        self.path("featuretable.txt")
        self.path("ca-bundle.crt")

        # Media plugins - CEF
        if self.prefix(src='../media_plugins/cef/%s' % self.args['configuration'], dst="llplugin"):
            self.path("media_plugin_cef.dll")
            self.end_prefix()

        # Media plugins - LibVLC
        if self.prefix(src='../media_plugins/libvlc/%s' % self.args['configuration'], dst="llplugin"):
            self.path("media_plugin_libvlc.dll")
            self.end_prefix()

        # CEF runtime files - debug
        if self.args['configuration'].lower() == 'debug':
            if self.prefix(src=os.path.join(os.pardir, 'packages', 'bin', 'debug'), dst="llplugin"):
                self.path("chrome_elf.dll")
                self.path("d3dcompiler_47.dll")
                self.path("libcef.dll")
                self.path("libEGL.dll")
                self.path("libGLESv2.dll")
                self.path("dullahan_host.exe")
                self.path("natives_blob.bin")
                self.path("snapshot_blob.bin")
                self.path("v8_context_snapshot.bin")
                self.path("widevinecdmadapter.dll")
                self.end_prefix()
        else:
        # CEF runtime files - not debug (release, relwithdebinfo etc.)
            if self.prefix(src=os.path.join(os.pardir, 'packages', 'bin', 'release'), dst="llplugin"):
                self.path("chrome_elf.dll")
                self.path("d3dcompiler_47.dll")
                self.path("libcef.dll")
                self.path("libEGL.dll")
                self.path("libGLESv2.dll")
                self.path("dullahan_host.exe")
                self.path("natives_blob.bin")
                self.path("snapshot_blob.bin")
                self.path("v8_context_snapshot.bin")
                self.path("widevinecdmadapter.dll")
                self.end_prefix()

        # CEF files common to all configurations
        if self.prefix(src=os.path.join(os.pardir, 'packages', 'resources'), dst="llplugin"):
            self.path("cef.pak")
            self.path("cef_100_percent.pak")
            self.path("cef_200_percent.pak")
            self.path("cef_extensions.pak")
            self.path("devtools_resources.pak")
            self.path("icudtl.dat")
            self.end_prefix()

        if self.prefix(src=os.path.join(os.pardir, 'packages', 'resources', 'locales'), dst=os.path.join('llplugin', 'locales')):
            self.path("am.pak")
            self.path("ar.pak")
            self.path("bg.pak")
            self.path("bn.pak")
            self.path("ca.pak")
            self.path("cs.pak")
            self.path("da.pak")
            self.path("de.pak")
            self.path("el.pak")
            self.path("en-GB.pak")
            self.path("en-US.pak")
            self.path("es-419.pak")
            self.path("es.pak")
            self.path("et.pak")
            self.path("fa.pak")
            self.path("fi.pak")
            self.path("fil.pak")
            self.path("fr.pak")
            self.path("gu.pak")
            self.path("he.pak")
            self.path("hi.pak")
            self.path("hr.pak")
            self.path("hu.pak")
            self.path("id.pak")
            self.path("it.pak")
            self.path("ja.pak")
            self.path("kn.pak")
            self.path("ko.pak")
            self.path("lt.pak")
            self.path("lv.pak")
            self.path("ml.pak")
            self.path("mr.pak")
            self.path("ms.pak")
            self.path("nb.pak")
            self.path("nl.pak")
            self.path("pl.pak")
            self.path("pt-BR.pak")
            self.path("pt-PT.pak")
            self.path("ro.pak")
            self.path("ru.pak")
            self.path("sk.pak")
            self.path("sl.pak")
            self.path("sr.pak")
            self.path("sv.pak")
            self.path("sw.pak")
            self.path("ta.pak")
            self.path("te.pak")
            self.path("th.pak")
            self.path("tr.pak")
            self.path("uk.pak")
            self.path("vi.pak")
            self.path("zh-CN.pak")
            self.path("zh-TW.pak")
            self.end_prefix()

        if self.prefix(src=os.path.join(os.pardir, 'packages', 'bin', 'release'), dst="llplugin"):
            self.path("libvlc.dll")
            self.path("libvlccore.dll")
            self.path("plugins/")
            self.end_prefix()

        # pull in the crash logger and updater from other projects
        # tag:"crash-logger" here as a cue to the exporter
        self.path(src='../win_crash_logger/%s/windows-crash-logger.exe' % self.args['configuration'],
                  dst="win_crash_logger.exe")

        if not self.is_packaging_viewer():
            self.package_file = "copied_deps"

    def nsi_file_commands(self, install=True):
        def wpath(path):
            if path.endswith('/') or path.endswith(os.path.sep):
                path = path[:-1]
            path = path.replace('/', '\\')
            return path

        result = ""
        dest_files = [pair[1] for pair in self.file_list if pair[0] and os.path.isfile(pair[1])]
        # sort deepest hierarchy first
        dest_files.sort(lambda a,b: cmp(a.count(os.path.sep),b.count(os.path.sep)) or cmp(a,b))
        dest_files.reverse()
        out_path = None
        for pkg_file in dest_files:
            rel_file = os.path.normpath(pkg_file.replace(self.get_dst_prefix()+os.path.sep,''))
            installed_dir = wpath(os.path.join('$INSTDIR', os.path.dirname(rel_file)))
            pkg_file = wpath(os.path.normpath(pkg_file))
            if installed_dir != out_path:
                if install:
                    out_path = installed_dir
                    result += 'SetOutPath ' + out_path + '\n'
            if install:
                result += 'File ' + pkg_file + '\n'
            else:
                result += 'Delete ' + wpath(os.path.join('$INSTDIR', rel_file)) + '\n'

        # at the end of a delete, just rmdir all the directories
        if not install:
            deleted_file_dirs = [os.path.dirname(pair[1].replace(self.get_dst_prefix()+os.path.sep,'')) for pair in self.file_list]
            # find all ancestors so that we don't skip any dirs that happened to have no non-dir children
            deleted_dirs = []
            for d in deleted_file_dirs:
                deleted_dirs.extend(path_ancestors(d))
            # sort deepest hierarchy first
            deleted_dirs.sort(lambda a,b: cmp(a.count(os.path.sep),b.count(os.path.sep)) or cmp(a,b))
            deleted_dirs.reverse()
            prev = None
            for d in deleted_dirs:
                if d != prev:   # skip duplicates
                    result += 'RMDir ' + wpath(os.path.join('$INSTDIR', os.path.normpath(d))) + '\n'
                prev = d

        return result
	
    def sign_command(self, *argv):
        return [
            "signtool.exe", "sign", "/v",
            "/n", self.args['signature'],
            "/p", os.environ['VIEWER_SIGNING_PWD'],
            "/d","%s" % self.channel(),
            "/t","http://timestamp.comodoca.com/authenticode"
        ] + list(argv)
	
    def sign(self, *argv):
        subprocess.check_call(self.sign_command(*argv))

    def package_finish(self):
        if 'signature' in self.args and 'VIEWER_SIGNING_PWD' in os.environ:
            try:
                self.sign(self.args['configuration']+"\\"+self.final_exe())
                self.sign(self.args['configuration']+"\\AlchemyPlugin.exe")
                self.sign(self.args['configuration']+"\\SLVoice.exe")
            except:
                print "Couldn't sign binaries. Tried to sign %s" % self.args['configuration'] + "\\" + self.final_exe()
		
        # a standard map of strings for replacing in the templates
        substitution_strings = {
            'version' : '.'.join(self.args['version']),
            'version_short' : '.'.join(self.args['version'][:-1]),
            'version_dashes' : '-'.join(self.args['version']),
            'final_exe' : self.final_exe(),
            'flags':'',
            'app_name':self.app_name(),
            'app_name_oneword':self.app_name_oneword()
            }

        installer_file = self.installer_base_name() + '_Setup.exe'
        substitution_strings['installer_file'] = installer_file

        version_vars = """
        !define INSTEXE  "%(final_exe)s"
        !define VERSION "%(version_short)s"
        !define VERSION_LONG "%(version)s"
        !define VERSION_DASHES "%(version_dashes)s"
        """ % substitution_strings

        if self.channel_type() == 'release':
            substitution_strings['caption'] = CHANNEL_VENDOR_BASE
        else:
            substitution_strings['caption'] = self.app_name() + ' ${VERSION}'

        inst_vars_template = """
            !define INSTOUTFILE "%(installer_file)s"
            !define INSTEXE  "%(final_exe)s"
            !define APPNAME   "%(app_name)s"
            !define APPNAMEONEWORD   "%(app_name_oneword)s"
            !define VERSION "%(version_short)s"
            !define VERSION_LONG "%(version)s"
            !define VERSION_DASHES "%(version_dashes)s"
            !define URLNAME   "secondlife"
            !define CAPTIONSTR "%(caption)s"
            !define VENDORSTR "Singularity Viewer Project"
            """

        tempfile = "%s_setup_tmp.nsi" % self.viewer_branding_id()
        # the following replaces strings in the nsi template
        # it also does python-style % substitution
        self.replace_in("installers/windows/installer_template.nsi", tempfile, {
                "%%VERSION%%":version_vars,
                "%%SOURCE%%":self.get_src_prefix(),
                "%%INST_VARS%%":inst_vars_template % substitution_strings,
                "%%INSTALL_FILES%%":self.nsi_file_commands(True),
                "%%DELETE_FILES%%":self.nsi_file_commands(False),
                "%%WIN64_BIN_BUILD%%":"!define WIN64_BIN_BUILD 1" if self.is_win64() else "",
                })

        # We use the Unicode version of NSIS, available from
        # http://www.scratchpaper.com/
        try:
            import _winreg as reg
            NSIS_path = reg.QueryValue(reg.HKEY_LOCAL_MACHINE, r"SOFTWARE\NSIS") + '\\makensis.exe'
            self.run_command([proper_windows_path(NSIS_path), self.dst_path_of(tempfile)])
        except:
            try:
                NSIS_path = os.environ['ProgramFiles'] + '\\NSIS\\makensis.exe'
                self.run_command([proper_windows_path(NSIS_path), self.dst_path_of(tempfile)])
            except:
                NSIS_path = os.environ['ProgramFiles(X86)'] + '\\NSIS\\makensis.exe'
                self.run_command([proper_windows_path(NSIS_path),self.dst_path_of(tempfile)])


        # self.remove(self.dst_path_of(tempfile))
        if 'signature' in self.args and 'VIEWER_SIGNING_PWD' in os.environ:
            try:
                self.sign(self.args['configuration'] + "\\" + substitution_strings['installer_file'])
            except: 
                print "Couldn't sign windows installer. Tried to sign %s" % self.args['configuration'] + "\\" + substitution_strings['installer_file']

        self.created_path(self.dst_path_of(installer_file))
        self.package_file = installer_file


class Windows_i686_Manifest(WindowsManifest):
    def construct(self):
        super(Windows_i686_Manifest, self).construct()

        # Get shared libs from the shared libs staging directory
        if self.prefix(src=os.path.join(os.pardir, 'sharedlibs', self.args['configuration']),
                       dst=""):

            # Security
            self.path("libcrypto-1_1.dll")
            self.path("libssl-1_1.dll")

            # Get fmod studio dll, continue if missing
            try:
                if self.args['configuration'].lower() == 'debug':
                    self.path("fmodL.dll")
                else:
                    self.path("fmod.dll")
            except:
                print "Skipping fmodstudio audio library(assuming other audio engine)"

            self.end_prefix()

        if self.prefix(src=os.path.join(self.args['build'], os.pardir, 'packages', 'bin'), dst="redist"):
            self.path("vc_redist.x86.exe")
            self.end_prefix()


class Windows_x86_64_Manifest(WindowsManifest):
    def construct(self):
        super(Windows_x86_64_Manifest, self).construct()

        # Get shared libs from the shared libs staging directory
        if self.prefix(src=os.path.join(os.pardir, 'sharedlibs', self.args['configuration']),
                       dst=""):

            # Security
            self.path("libcrypto-1_1-x64.dll")
            self.path("libssl-1_1-x64.dll")

            # Get fmodstudio dll, continue if missing
            try:
                if self.args['configuration'].lower() == 'debug':
                    self.path("fmodL64.dll")
                else:
                    self.path("fmod64.dll")
            except:
                print "Skipping fmodstudio audio library(assuming other audio engine)"

            self.end_prefix()

        if self.prefix(src=os.path.join(self.args['build'], os.pardir, 'packages', 'bin'), dst="redist"):
            self.path("vc_redist.x64.exe")
            self.end_prefix()


class DarwinManifest(ViewerManifest):
    build_data_json_platform = 'mac'

    def finish_build_data_dict(self, build_data_dict):
        build_data_dict.update({'Bundle Id':self.args['bundleid']})
        return build_data_dict

    def is_packaging_viewer(self):
        # darwin requires full app bundle packaging even for debugging.
        return True

    def construct(self):
        # copy over the build result (this is a no-op if run within the xcode script)
        self.path(self.args['configuration'] + "/" + self.app_name() + ".app", dst="")

        pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
        relpkgdir = os.path.join(pkgdir, "lib", "release")
        debpkgdir = os.path.join(pkgdir, "lib", "debug")
        relbinpkgdir = os.path.join(pkgdir, "bin", "release")

        with self.prefix(src="", dst="Contents"):  # everything goes in Contents
            self.path("Info.plist", dst="Info.plist")

            # copy additional libs in <bundle>/Contents/MacOS/
            self.path(os.path.join(relpkgdir, "libndofdev.dylib"), dst="Resources/libndofdev.dylib")

            # Make a symlink to a nested app Frameworks directory that doesn't
            # yet exist. We shouldn't need this; the only things that need
            # Frameworks are nested apps under viewer_app, and they should
            # simply find its Contents/Frameworks by relative pathnames. But
            # empirically, we do: if we omit this symlink, CEF doesn't work --
            # the login splash screen doesn't even display. SIIIIGH.
            # We're passing a path that's already relative, hence symlinkf()
            # rather than relsymlinkf().
            self.symlinkf(os.path.join("Resources", "Frameworks"))

            # most everything goes in the Resources directory
            with self.prefix(src="", dst="Resources"):
                super(DarwinManifest, self).construct()

                with self.prefix("cursors_mac"):
                    self.path("*.tif")

                self.path("featuretable_mac.txt")
                self.path("ca-bundle.crt")

                icon_path = self.icon_path()
                with self.prefix(src=icon_path, dst="") :
                    self.path("%s.icns" % self.viewer_branding_id())

                self.path("%s.nib" % self.viewer_branding_id())

                # Translations
                self.path("English.lproj/language.txt")
                self.replace_in(src="English.lproj/InfoPlist.strings",
                                dst="English.lproj/InfoPlist.strings",
                                searchdict={'%%VERSION%%':'.'.join(self.args['version'])}
                                )
                self.path("German.lproj")
                self.path("Japanese.lproj")
                self.path("Korean.lproj")
                self.path("da.lproj")
                self.path("es.lproj")
                self.path("fr.lproj")
                self.path("hu.lproj")
                self.path("it.lproj")
                self.path("nl.lproj")
                self.path("pl.lproj")
                self.path("pt.lproj")
                self.path("ru.lproj")
                self.path("tr.lproj")
                self.path("uk.lproj")
                self.path("zh-Hans.lproj")

                def path_optional(src, dst):
                    """
                    For a number of our self.path() calls, not only do we want
                    to deal with the absence of src, we also want to remember
                    which were present. Return either an empty list (absent)
                    or a list containing dst (present). Concatenate these
                    return values to get a list of all libs that are present.
                    """
                    # This was simple before we started needing to pass
                    # wildcards. Fortunately, self.path() ends up appending a
                    # (source, dest) pair to self.file_list for every expanded
                    # file processed. Remember its size before the call.
                    oldlen = len(self.file_list)
                    self.path(src, dst)
                    # The dest appended to self.file_list has been prepended
                    # with self.get_dst_prefix(). Strip it off again.
                    added = [os.path.relpath(d, self.get_dst_prefix())
                             for s, d in self.file_list[oldlen:]]
                    if not added:
                        print "Skipping %s" % dst
                    return added

                # dylibs is a list of all the .dylib files we expect to need
                # in our bundled sub-apps. For each of these we'll create a
                # symlink from sub-app/Contents/Resources to the real .dylib.
                # Need to get the llcommon dll from any of the build directories as well.
                libfile = "libllcommon.dylib"
                dylibs = path_optional(self.find_existing_file(os.path.join(os.pardir,
                                                               "llcommon",
                                                               self.args['configuration'],
                                                               libfile),
                                                               os.path.join(relpkgdir, libfile)),
                                       dst=libfile)

                for libfile in (
                                "libapr-1.0.dylib",
                                "libaprutil-1.0.dylib",
                                "libexception_handler.dylib",
                                "libGLOD.dylib",
                                "libopenjpeg.dylib",
                                "libfreetype.6.dylib",
                                ):
                    dylibs += path_optional(os.path.join(relpkgdir, libfile), libfile)

                # SLVoice and vivox lols, no symlinks needed
                for libfile in (
                            'libortp.dylib',
                            'libsndfile.dylib',
                            'libvivoxoal.dylib',
                            'libvivoxsdk.dylib',
                            'libvivoxplatform.dylib',
                            'ca-bundle.crt',
                            'SLVoice',
                            ):
                    self.path2basename(relpkgdir, libfile)
                
                # dylibs that vary based on configuration
                if self.args['configuration'].lower() == 'debug':
                    for libfile in (
                                "libfmodL.dylib",
                                ):
                        dylibs += path_optional(os.path.join(debpkgdir, libfile), libfile)
                else:
                    for libfile in (
                                "libfmod.dylib",
                                ):
                        dylibs += path_optional(os.path.join(relpkgdir, libfile), libfile)

                # our apps
                for app_bld_dir, app in (("mac_crash_logger", "mac-crash-logger.app"),
                                         # plugin launcher
                                         (os.path.join("llplugin", "slplugin"), "AlchemyPlugin.app"),
                                         ):
                    self.path2basename(os.path.join(os.pardir,
                                                    app_bld_dir, self.args['configuration']),
                                       app)

                    # our apps dependencies on shared libs
                    # for each app, for each dylib we collected in dylibs,
                    # create a symlink to the real copy of the dylib.
                    resource_path = self.dst_path_of(os.path.join(app, "Contents", "Resources"))
                    for libfile in dylibs:
                        src = os.path.join(os.pardir, os.pardir, os.pardir, libfile)
                        dst = os.path.join(resource_path, libfile)
                        try:
                            self.symlinkf(src, dst)
                        except OSError as err:
                            print "Can't symlink %s -> %s: %s" % (src, dst, err)

                # Dullahan helper apps go inside AlchemyPlugin.app
                with self.prefix(src="", dst="AlchemyPlugin.app/Contents/Frameworks"):
                    helperappfile = 'DullahanHelper.app'
                    self.path2basename(relbinpkgdir, helperappfile)

                    pluginframeworkpath = self.dst_path_of('Chromium Embedded Framework.framework');
                    # Putting a Frameworks directory under Contents/MacOS
                    # isn't canonical, but the path baked into Dullahan
                    # Helper.app/Contents/MacOS/DullahanHelper is:
                    # @executable_path/Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework
                    # (notice, not @executable_path/../Frameworks/etc.)
                    # So we'll create a symlink (below) from there back to the
                    # Frameworks directory nested under SLPlugin.app.
                    helperframeworkpath = \
                        self.dst_path_of('DullahanHelper.app/Contents/MacOS/'
                                         'Frameworks/Chromium Embedded Framework.framework')

                    helperexecutablepath = self.dst_path_of('DullahanHelper.app/Contents/MacOS/DullahanHelper')
                    self.run_command(['install_name_tool', '-change',
                                     '@rpath/Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework',
                                     '@executable_path/Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework', helperexecutablepath])


                # SLPlugin plugins
                with self.prefix(src="", dst="llplugin"):
                    self.path2basename("../media_plugins/cef/" + self.args['configuration'],
                                       "media_plugin_cef.dylib")

                    # copy LibVLC plugin itself
                    self.path2basename("../media_plugins/libvlc/" + self.args['configuration'],
                                       "media_plugin_libvlc.dylib")

                    # copy LibVLC dynamic libraries
                    with self.prefix(src=os.path.join(os.pardir, 'packages', 'lib', 'release' ), dst="lib"):
                        self.path( "libvlc*.dylib*" )

                    # copy LibVLC plugins folder
                    with self.prefix(src=os.path.join(os.pardir, 'packages', 'lib', 'release', 'plugins' ), dst="lib"):
                        self.path( "*.dylib" )
                        self.path( "plugins.dat" )

                    # do this install_name_tool *after* media plugin is copied over
                    dylibexecutablepath = self.dst_path_of('media_plugin_cef.dylib')
                    self.run_command(['install_name_tool', '-change',
                                     '@rpath/Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework',
                                     '@executable_path/../Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework', dylibexecutablepath])


                # CEF framework goes inside Alchemy.app/Contents/Frameworks
                with self.prefix(src="", dst="Frameworks"):
                    frameworkfile="Chromium Embedded Framework.framework"
                    self.path2basename(relbinpkgdir, frameworkfile)

                # This code constructs a relative path from the
                # target framework folder back to the location of the symlink.
                # It needs to be relative so that the symlink still works when
                # (as is normal) the user moves the app bundle out of the DMG
                # and into the /Applications folder. Note we also call 'raise'
                # to terminate the process if we get an error since without
                # this symlink, Second Life web media can't possibly work.
                # Real Framework folder:
                #   Alchemy.app/Contents/Frameworks/Chromium Embedded Framework.framework/
                # Location of symlink and why it's relative 
                #   Alchemy.app/Contents/Resources/AlchemyPlugin.app/Contents/Frameworks/Chromium Embedded Framework.framework/
                # Real Frameworks folder, with the symlink inside the bundled SLPlugin.app (and why it's relative)
                #   <top level>.app/Contents/Frameworks/Chromium Embedded Framework.framework/
                #   <top level>.app/Contents/Resources/AlchemyPlugin.app/Contents/Frameworks/Chromium Embedded Framework.framework ->
                # It might seem simpler just to create a symlink Frameworks to
                # the parent of Chromimum Embedded Framework.framework. But
                # that would create a symlink cycle, which breaks our
                # packaging step. So make a symlink from Chromium Embedded
                # Framework.framework to the directory of the same name, which
                # is NOT an ancestor of the symlink.
                frameworkpath = os.path.join(os.pardir, os.pardir, os.pardir, 
                                             os.pardir, "Frameworks",
                                             "Chromium Embedded Framework.framework")
                try:
                    # from AlchemyPlugin.app/Contents/Frameworks/Chromium Embedded
                    # Framework.framework back to
                    # Alchemy.app/Contents/Frameworks/Chromium Embedded Framework.framework
                    origin, target = pluginframeworkpath, frameworkpath
                    self.symlinkf(target, origin)
                    # from AlchemyPlugin.app/Contents/Frameworks/Dullahan
                    # Helper.app/Contents/MacOS/Frameworks/Chromium Embedded
                    # Framework.framework back to
                    # AlchemyPlugin.app/Contents/Frameworks/Chromium Embedded Framework.framework
                    self.cmakedirs(os.path.dirname(helperframeworkpath))
                    origin = helperframeworkpath
                    target = os.path.join(os.pardir, frameworkpath)
                    self.symlinkf(target, origin)
                except OSError as err:
                    print "Can't symlink %s -> %s: %s" % (origin, target, err)
                    raise


        # NOTE: the -S argument to strip causes it to keep enough info for
        # annotated backtraces (i.e. function names in the crash log).  'strip' with no
        # arguments yields a slightly smaller binary but makes crash logs mostly useless.
        # This may be desirable for the final release.  Or not.
        if ("package" in self.args['actions'] or
            "unpacked" in self.args['actions']):
            self.run_command('strip -S %(viewer_binary)r' %
                             { 'viewer_binary' : self.dst_path_of('Contents/MacOS/%s' % self.viewer_branding_id())})

    def copy_finish(self):
        # Force executable permissions to be set for scripts
        # see CHOP-223 and http://mercurial.selenic.com/bts/issue1802
        pass
        # for script in 'Contents/MacOS/update_install.py',:
        #     self.run_command("chmod +x %r" % os.path.join(self.get_dst_prefix(), script))

    def app_name(self):
        return self.channel_oneword()

    def package_finish(self):
        global CHANNEL_VENDOR_BASE
        # MBW -- If the mounted volume name changes, it breaks the .DS_Store's background image and icon positioning.
        #  If we really need differently named volumes, we'll need to create multiple DS_Store file images, or use some other trick.

        volname=CHANNEL_VENDOR_BASE+" Installer"  # DO NOT CHANGE without understanding comment above

        imagename = self.installer_base_name()

        sparsename = imagename + ".sparseimage"
        finalname = imagename + ".dmg"
        # make sure we don't have stale files laying about
        self.remove(sparsename, finalname)

        self.run_command(['hdiutil', 'create', sparsename,
                          '-volname', volname, '-fs', 'HFS+',
                          '-type', 'SPARSE', '-megabytes', '1300',
                          '-layout', 'SPUD'])

        # mount the image and get the name of the mount point and device node
        try:
            hdi_output = subprocess.check_output(['hdiutil', 'attach', '-private', sparsename])
        except subprocess.CalledProcessError as err:
            sys.exit("failed to mount image at '%s'" % sparsename)

        try:
            devfile = re.search("/dev/disk([0-9]+)[^s]", hdi_output).group(0).strip()
            volpath = re.search('HFS\s+(.+)', hdi_output).group(1).strip()

            if devfile != '/dev/disk1':
                # adding more debugging info based upon nat's hunches to the
                # logs to help track down 'SetFile -a V' failures -brad
                print "WARNING: 'SetFile -a V' command below is probably gonna fail"

            # Copy everything in to the mounted .dmg

            app_name = self.app_name()

            # Hack:
            # Because there is no easy way to coerce the Finder into positioning
            # the app bundle in the same place with different app names, we are
            # adding multiple .DS_Store files to svn. There is one for release,
            # one for release candidate and one for first look. Any other channels
            # will use the release .DS_Store, and will look broken.
            # - Ambroff 2008-08-20
            dmg_template = os.path.join(
                'installers', 'darwin', '%s-dmg' % self.channel_type())

            if not os.path.exists (self.src_path_of(dmg_template)):
                dmg_template = os.path.join ('installers', 'darwin', 'release-dmg')

            for s,d in {self.get_dst_prefix():app_name + ".app",
                        os.path.join(dmg_template, "_VolumeIcon.icns"): ".VolumeIcon.icns",
                        os.path.join(dmg_template, "background.jpg"): "background.jpg",
                        os.path.join(dmg_template, "_DS_Store"): ".DS_Store"}.items():
                print "Copying to dmg", s, d
                self.copy_action(self.src_path_of(s), os.path.join(volpath, d))

            # Hide the background image, DS_Store file, and volume icon file (set their "visible" bit)
            for f in ".VolumeIcon.icns", "background.jpg", ".DS_Store":
                pathname = os.path.join(volpath, f)
                # We've observed mysterious "no such file" failures of the SetFile
                # command, especially on the first file listed above -- yet
                # subsequent inspection of the target directory confirms it's
                # there. Timing problem with copy command? Try to handle.
                for x in xrange(3):
                    if os.path.exists(pathname):
                        print "Confirmed existence: %r" % pathname
                        break
                    print "Waiting for %s copy command to complete (%s)..." % (f, x+1)
                    sys.stdout.flush()
                    time.sleep(1)
                # If we fall out of the loop above without a successful break, oh
                # well, possibly we've mistaken the nature of the problem. In any
                # case, don't hang up the whole build looping indefinitely, let
                # the original problem manifest by executing the desired command.
                self.run_command(['SetFile', '-a', 'V', pathname])

            # Create the alias file (which is a resource file) from the .r
            self.run_command(
                ['Rez', self.src_path_of("installers/darwin/release-dmg/Applications-alias.r"),
                 '-o', os.path.join(volpath, "Applications")])

            # Set the alias file's alias and custom icon bits
            self.run_command(['SetFile', '-a', 'AC', os.path.join(volpath, "Applications")])

            # Set the disk image root's custom icon bit
            self.run_command(['SetFile', '-a', 'C', volpath])

            # Sign the app if requested; 
            # do this in the copy that's in the .dmg so that the extended attributes used by 
            # the signature are preserved; moving the files using python will leave them behind
            # and invalidate the signatures.
            if 'signature' in self.args:
                app_in_dmg=os.path.join(volpath,self.app_name()+".app")
                print "Attempting to sign '%s'" % app_in_dmg
                identity = self.args['signature']
                if identity == '':
                    identity = 'Developer ID Application'

                # Look for an environment variable set via build.sh when running in Team City.
                try:
                    build_secrets_checkout = os.environ['build_secrets_checkout']
                except KeyError:
                    pass
                else:
                    # variable found so use it to unlock keychain followed by codesign
                    home_path = os.environ['HOME']
                    keychain_pwd_path = os.path.join(build_secrets_checkout,'code-signing-osx','password.txt')
                    keychain_pwd = open(keychain_pwd_path).read().rstrip()

                    # Note: As of macOS Sierra, keychains are created with names postfixed with '-db' so for example, the
                    #       SL Viewer keychain would by default be found in ~/Library/Keychains/viewer.keychain-db instead of
                    #       just ~/Library/Keychains/viewer.keychain in earlier versions.
                    #
                    #       Because we have old OS files from previous versions of macOS on the build hosts, the configurations
                    #       are different on each host. Some have viewer.keychain, some have viewer.keychain-db and some have both.
                    #       As you can see in the line below, this script expects the Linden Developer cert/keys to be in viewer.keychain.
                    #
                    #       To correctly sign builds you need to make sure ~/Library/Keychains/viewer.keychain exists on the host
                    #       and that it contains the correct cert/key. If a build host is set up with a clean version of macOS Sierra (or later)
                    #       then you will need to change this line (and the one for 'codesign' command below) to point to right place or else
                    #       pull in the cert/key into the default viewer keychain 'viewer.keychain-db' and export it to 'viewer.keychain'
                    viewer_keychain = os.path.join(home_path, 'Library',
                                                   'Keychains', 'viewer.keychain')
                    self.run_command(['security', 'unlock-keychain',
                                      '-p', keychain_pwd, viewer_keychain])
                    signed=False
                    sign_attempts=3
                    sign_retry_wait=15
                    while (not signed) and (sign_attempts > 0):
                        try:
                            sign_attempts-=1;
                            self.run_command(
                                # Note: See blurb above about names of keychains
                               ['codesign', '--verbose', '--deep', '--force',
                                '--keychain', viewer_keychain, '--sign', identity,
                                app_in_dmg])
                            signed=True # if no exception was raised, the codesign worked
                        except ManifestError as err:
                            if sign_attempts:
                                print >> sys.stderr, "codesign failed, waiting %d seconds before retrying" % sign_retry_wait
                                time.sleep(sign_retry_wait)
                                sign_retry_wait*=2
                            else:
                                print >> sys.stderr, "Maximum codesign attempts exceeded; giving up"
                                raise
                    self.run_command(['spctl', '-a', '-texec', '-vv', app_in_dmg])

            imagename= self.app_name() + "_" + '_'.join(self.args['version'])


        finally:
            # Unmount the image even if exceptions from any of the above 
            self.run_command(['hdiutil', 'detach', '-force', devfile])

        print "Converting temp disk image to final disk image"
        self.run_command(['hdiutil', 'convert', sparsename, '-format', 'UDZO',
                          '-imagekey', 'zlib-level=9', '-o', finalname])
        self.run_command(['hdiutil', 'internet-enable', '-yes', finalname])
        # get rid of the temp file
        self.package_file = finalname
        self.remove(sparsename)


class Darwin_i386_Manifest(DarwinManifest):
    address_size = 32


class Darwin_i686_Manifest(DarwinManifest):
    """alias in case arch is passed as i686 instead of i386"""
    pass


class Darwin_x86_64_Manifest(DarwinManifest):
    address_size = 64


class LinuxManifest(ViewerManifest):
    def is_packaging_viewer(self):
        super(LinuxManifest, self).is_packaging_viewer()
        return True # We always want a packaged viewer even without archive.

    def do(self, *actions):
        super(LinuxManifest, self).do(*actions)
        if not 'package' in self.actions:
            self.package_finish() # Always finish the package.
        else:
            # package_finish() was called by super.do() so just create the TAR.
            self.create_archive()
        return self.file_list
    
    def construct(self):
        import shutil
        shutil.rmtree("./packaged/app_settings/shaders", ignore_errors=True);
        super(LinuxManifest, self).construct()

        self.path("licenses-linux.txt","licenses.txt")

        pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
        relpkgdir = os.path.join(pkgdir, "lib", "release")
        debpkgdir = os.path.join(pkgdir, "lib", "debug")

        if self.prefix("linux_tools", dst=""):
            self.path("client-readme.txt","README-linux.txt")
            self.path("client-readme-voice.txt","README-linux-voice.txt")
            self.path("client-readme-joystick.txt","README-linux-joystick.txt")
            self.path("wrapper.sh",self.viewer_branding_id())
            if self.prefix(src="", dst="etc"):
                self.path("handle_secondlifeprotocol.sh")
                self.path("register_secondlifeprotocol.sh")
                self.path("refresh_desktop_app_entry.sh")
                self.path("launch_url.sh")
                self.end_prefix("etc")
            self.path("install.sh")
            self.end_prefix("linux_tools")

        if self.prefix(src="", dst="bin"):
            self.path("%s-bin"%self.viewer_branding_id(),"do-not-directly-run-%s-bin"%self.viewer_branding_id())
            self.path("../linux_crash_logger/linux-crash-logger","linux-crash-logger.bin")
            self.path2basename("../llplugin/slplugin", "AlchemyPlugin")
            self.end_prefix("bin")

        if self.prefix("res-sdl"):
            self.path("*")
            # recurse
            self.end_prefix("res-sdl")

        # Get the icons based on the channel type
        icon_path = self.icon_path()
        print "DEBUG: icon_path '%s'" % icon_path
        if self.prefix(src=icon_path, dst="") :
            self.path("viewer.png","viewer_icon.png")
            if self.prefix(src="",dst="res-sdl") :
                self.path("viewer_256.BMP","viewer_icon.BMP")
                self.end_prefix("res-sdl")
            self.end_prefix(icon_path)

        # plugins
        if self.prefix(src="", dst="bin/llplugin"):
            self.path("../media_plugins/gstreamer010/libmedia_plugin_gstreamer010.so", "libmedia_plugin_gstreamer.so")
            self.path("../media_plugins/libvlc/libmedia_plugin_libvlc.so", "libmedia_plugin_libvlc.so")
            self.path("../media_plugins/cef/libmedia_plugin_cef.so", "libmedia_plugin_cef.so")
            self.end_prefix("bin/llplugin")

        if self.prefix(src=os.path.join(os.pardir, 'packages', 'lib', 'vlc', 'plugins'), dst="bin/llplugin/vlc/plugins"):
            self.path( "plugins.dat" )
            self.path( "*/*.so" )
            self.end_prefix()

        if self.prefix(src=os.path.join(os.pardir, 'packages', 'lib' ), dst="lib"):
            self.path( "libvlc*.so*" )
            self.end_prefix()

        # CEF files 
        if self.prefix(src=os.path.join(pkgdir, 'bin', 'release'), dst="bin"):
            self.path("chrome-sandbox")
            self.path("dullahan_host")
            self.path("natives_blob.bin")
            self.path("snapshot_blob.bin")
            self.path("v8_context_snapshot.bin")
            self.end_prefix()

        if self.prefix(src=os.path.join(pkgdir, 'resources'), dst="bin"):
            self.path("cef.pak")
            self.path("cef_extensions.pak")
            self.path("cef_100_percent.pak")
            self.path("cef_200_percent.pak")
            self.path("devtools_resources.pak")
            self.path("icudtl.dat")
            self.end_prefix()

        if self.prefix(src=os.path.join(pkgdir, 'resources', 'locales'), dst=os.path.join('bin', 'locales')):
            self.path("am.pak")
            self.path("ar.pak")
            self.path("bg.pak")
            self.path("bn.pak")
            self.path("ca.pak")
            self.path("cs.pak")
            self.path("da.pak")
            self.path("de.pak")
            self.path("el.pak")
            self.path("en-GB.pak")
            self.path("en-US.pak")
            self.path("es.pak")
            self.path("es-419.pak")
            self.path("et.pak")
            self.path("fa.pak")
            self.path("fi.pak")
            self.path("fil.pak")
            self.path("fr.pak")
            self.path("gu.pak")
            self.path("he.pak")
            self.path("hi.pak")
            self.path("hr.pak")
            self.path("hu.pak")
            self.path("id.pak")
            self.path("it.pak")
            self.path("ja.pak")
            self.path("kn.pak")
            self.path("ko.pak")
            self.path("lt.pak")
            self.path("lv.pak")
            self.path("ml.pak")
            self.path("mr.pak")
            self.path("ms.pak")
            self.path("nb.pak")
            self.path("nl.pak")
            self.path("pl.pak")
            self.path("pt-BR.pak")
            self.path("pt-PT.pak")
            self.path("ro.pak")
            self.path("ru.pak")
            self.path("sk.pak")
            self.path("sl.pak")
            self.path("sr.pak")
            self.path("sv.pak")
            self.path("sw.pak")
            self.path("ta.pak")
            self.path("te.pak")
            self.path("th.pak")
            self.path("tr.pak")
            self.path("uk.pak")
            self.path("vi.pak")
            self.path("zh-CN.pak")
            self.path("zh-TW.pak")
            self.end_prefix()

        # llcommon
        if not self.path("../llcommon/libllcommon.so", "lib/libllcommon.so"):
            print "Skipping llcommon.so (assuming llcommon was linked statically)"

        self.path("featuretable_linux.txt")
        self.path("ca-bundle.crt")

    def package_finish(self):

        self.strip_binaries()

        # Fix access permissions
        self.run_command("""
                find %(dst)s -type d | xargs --no-run-if-empty chmod 755;
                find %(dst)s -type f -perm 0700 | xargs --no-run-if-empty chmod 0755;
                find %(dst)s -type f -perm 0500 | xargs --no-run-if-empty chmod 0555;
                find %(dst)s -type f -perm 0600 | xargs --no-run-if-empty chmod 0644;
                find %(dst)s -type f -perm 0400 | xargs --no-run-if-empty chmod 0444;
                true""" %  {'dst':self.get_dst_prefix() })

    def create_archive(self):
        installer_name = self.installer_base_name()
        # temporarily move directory tree so that it has the right
        # name in the tarfile
        self.run_command("mv %(dst)s %(inst)s" % {
            'dst': self.get_dst_prefix(),
            'inst': self.build_path_of(installer_name)})
        try:
            # --numeric-owner hides the username of the builder for
            # security etc.
            self.run_command('tar -C %(dir)s --numeric-owner -cJf '
                             '%(inst_path)s.tar.xz %(inst_name)s' % {
                    'dir': self.get_build_prefix(),
                    'inst_name': installer_name,
                    'inst_path':self.build_path_of(installer_name)})
        finally:
            self.run_command("mv %(inst)s %(dst)s" % {
                'dst': self.get_dst_prefix(),
                'inst': self.build_path_of(installer_name)})
            self.package_file = installer_name + '.tar.xz'

    def strip_binaries(self):
        print "* Going strip-crazy on the packaged binaries"
        # makes some small assumptions about our packaged dir structure
        self.run_command(r"find %(d)r/lib %(d)r/lib32 %(d)r/lib64 -type f \! -name update_install | xargs --no-run-if-empty strip -S" % {'d': self.get_dst_prefix()} )
        self.run_command(r"find %(d)r/bin -executable -type f \! -name update_install | xargs --no-run-if-empty strip -S" % {'d': self.get_dst_prefix()} )


class Linux_i686_Manifest(LinuxManifest):
    def construct(self):
        super(Linux_i686_Manifest, self).construct()

        pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
        relpkgdir = os.path.join(pkgdir, "lib", "release")
        debpkgdir = os.path.join(pkgdir, "lib", "debug")

        if (not self.standalone()) and self.prefix(src=relpkgdir, dst="lib"):
            self.path("libapr-1.so")
            self.path("libapr-1.so.0")
            self.path("libapr-1.so.0.5.2")
            self.path("libaprutil-1.so")
            self.path("libaprutil-1.so.0")
            self.path("libaprutil-1.so.0.5.4")
            self.path("libcef.so")
            self.path("libexpat.so.*")
            self.path("libGLOD.so")
            self.path("libSDL-1.2.so.*")
            self.path("libopenjpeg.so*")
            self.path("libhunspell-1.6.so*")


            self.path("libtcmalloc_minimal.so.0")
            self.path("libtcmalloc_minimal.so.0.2.2")
            self.end_prefix("lib")

        # Vivox runtimes
        if self.prefix(src=relpkgdir, dst="bin"):
            self.path("SLVoice")
            self.end_prefix("bin")
        if self.prefix(src=relpkgdir, dst="lib"):
            self.path("libortp.so")
            self.path("libsndfile.so.1")
            self.path("libvivoxoal.so.1")
            self.path("libvivoxsdk.so")
            self.path("libvivoxplatform.so")
            self.end_prefix("lib")


class Linux_x86_64_Manifest(LinuxManifest):
    def construct(self):
        super(Linux_x86_64_Manifest, self).construct()

        pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
        relpkgdir = os.path.join(pkgdir, "lib", "release")
        debpkgdir = os.path.join(pkgdir, "lib", "debug")

        if (not self.standalone()) and self.prefix(relpkgdir, dst="lib64"):
            self.path("libapr-1.so*")
            self.path("libaprutil-1.so*")
            self.path("libexpat.so*")
            self.path("libGLOD.so")
            self.path("libSDL-1.2.so.*")
            self.path("libopenjpeg.so*")
            self.path("libhunspell-1.6.so*")
            self.path("libalut.so*")
            self.path("libopenal.so*")
            self.path("libfreetype.so*")

            try:
                self.path("libtcmalloc.so*") #formerly called google perf tools
                pass
            except:
                print "tcmalloc files not found, skipping"
                pass

            try:
                self.path("libfmod.so*")
                pass
            except:
                print "Skipping libfmod.so - not found"
                pass

            self.end_prefix("lib64")

        # Vivox runtimes
        if self.prefix(src=relpkgdir, dst="bin"):
            self.path("SLVoice")
            self.end_prefix("bin")
        if self.prefix(src=relpkgdir, dst="lib32"):
            self.path("libortp.so")
            self.path("libsndfile.so.1")
            self.path("libvivoxoal.so.1")
            self.path("libvivoxsdk.so")
            self.path("libvivoxplatform.so")
            self.end_prefix("lib32")

        # plugin runtime
        if self.prefix(src=relpkgdir, dst="lib64"):
            self.path("libcef.so")
            self.end_prefix("lib64")


################################################################

def symlinkf(src, dst):
    """
    Like ln -sf, but uses os.symlink() instead of running ln.
    """
    try:
        os.symlink(src, dst)
    except OSError, err:
        if err.errno != errno.EEXIST:
            raise
        # We could just blithely attempt to remove and recreate the target
        # file, but that strategy doesn't work so well if we don't have
        # permissions to remove it. Check to see if it's already the
        # symlink we want, which is the usual reason for EEXIST.
        elif os.path.islink(dst):
            if os.readlink(dst) == src:
                # the requested link already exists
                pass
            else:
                # dst is the wrong symlink; attempt to remove and recreate it
                os.remove(dst)
                os.symlink(src, dst)
        elif os.path.isdir(dst):
            print "Requested symlink (%s) exists but is a directory; replacing" % dst
            shutil.rmtree(dst)
            os.symlink(src, dst)
        elif os.path.exists(dst):
            print "Requested symlink (%s) exists but is a file; replacing" % dst
            os.remove(dst)
            os.symlink(src, dst)
        else:
            # see if the problem is that the parent directory does not exist
            # and try to explain what is missing
            (parent, tail) = os.path.split(dst)
            while not os.path.exists(parent):
                (parent, tail) = os.path.split(parent)
            if tail:
                raise Exception("Requested symlink (%s) cannot be created because %s does not exist"
                                % os.path.join(parent, tail))
            else:
                raise

if __name__ == "__main__":
    main()

