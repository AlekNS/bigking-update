#! /usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import config

VERSION='0.0.2'
APPNAME='BigKingUpdate'

top = '.'
out = 'build'

poco_incs=config.poco['incs']
poco_libs=config.poco['libs']
poco_libs_shared=config.poco['shared']

python_incs=config.python['incs']
python_libs=config.python['libs']
python_libs_shared=config.python['shared']


def options(opt):
    opt.load('compiler_cxx')
    opt.load('cxx')
    opt.load('compiler_c')
    opt.load('c')


def configure(conf):
    conf.load('compiler_cxx')
    conf.load('cxx')
    conf.load('compiler_c')
    conf.load('c')
    if sys.platform.find('win') < 0:
        conf.env.CXXFLAGS = '-g'
        conf.env.CFLAGS   = '-g'
    else:
        conf.env.append_unique('DEFINES', [
            "WIN32"
            "NDEBUG",
            "_WINDOWS",
            "POCO_STATIC",
         ])
        conf.env.CXXFLAGS   = ['/O1', '/Os',
                                '/GA', '/GF',
                                '/EHsc', '/MT']
        conf.env.LINKFLAGS    = ['/INCREMENTAL:NO', '/OPT:REF', '/OPT:ICF', '/MANIFEST']

    conf.env.append_unique('DEFINES', ['POCO_NO_AUTOMATIC_LIBS'])


def build(bld):
    includes = [
       '.',
    ]

    includes.extend(poco_incs)
    includes.extend(python_incs)

    libs = []
    libs.extend(poco_libs)
    libs.extend(python_libs)

    libs_shared = []
    libs.extend(poco_libs_shared)
    libs.extend(python_libs_shared)


    bld(
        export_includes = includes,
        name = 'includes',
    )


    libs_shared_link = []

    if sys.platform.find('win') < 0:
        libs_shared_link = [
            'pthread',
            'dl',
            'rt',
        ]
        bld(rule='cp ${SRC} ${TGT}', source="BigKingUpdate.xml.example", target="BigKingUpdate.xml")
    else:
        bld(rule='copy /Y ${SRC} ${TGT}', source="BigKingUpdate.xml.example", target="BigKingUpdate.xml")

    libs_shared_link.append(
        'python2.7'
    )


    libs_link = [
            'PocoUtil',
            'PocoZip',
            'PocoNet',
            'PocoXML',
            'PocoFoundation',
        ]

    if sys.platform.find('win') == 0:
        libs_link = [
                'iphlpapi',
                'winmm',
                'ws2_32',
                'Crypt32',
                'AdvAPI32',
                'PocoUtilmt',
                'PocoZipmt',
                'PocoNetmt',
                'PocoXMLmt',
                'PocoFoundationmt',
            ]


    #
    # Objects
    #
    bld.objects(
        target='common_objects',
        source=[
            'common/Version.cpp',
            'common/Software.cpp',
            'common/Configuration.cpp',
        ],
        use=['includes']
    )

    #
    # Update software
    #
    bld.program(
        target='BigKingUpdateSoftware',
        source=[
            'software/Downloader.cpp',
            'software/VersionInfo.cpp',
            'software/ProcessUpdate.cpp',
            'software/UpdateSoftware.cpp',
        ],

        stlibpath=libs,
        stlib    =libs_link,
        libpath  =libs_shared,
        lib      =libs_shared_link,

        use      = [
            'common_objects', 'includes',
        ]
    )


    #
    # Update package
    #


    if sys.platform.find('win') < 0:
        packPreprocDefs = [
            'XD3_POSIX=1',
            'XD3_WIN32=0',
        ]
    else:
        packPreprocDefs = [
            'XD3_POSIX=0',
            'XD3_WIN32=1',
            'XD3_STDIO=0',
        ]

    packPreprocDefs += [
        'GENERIC_ENCODE_TABLES=0',
        'REGRESSION_TEST=0',
        'SECONDARY_DJW=1', 
        'SECONDARY_FGK=1',
        'XD3_USE_LARGEFILE64=0',
        'XD3_DEBUG=0',
        'EXTERNAL_COMPRESSION=0',
        'SHELL_TESTS=0',
        'XD3_BUILD_FAST=1',
    ]

    bld.objects(
        target='xdelta_objects',
        source=[
            'xdelta/xdelta3.c',
        ],
        use=['includes']
    ).env.append_unique('DEFINES', packPreprocDefs)

    bld.program(
        target='BigKingUpdatePackage',
        source=[
            'package/Command.cpp',
            'package/CommandsContext.cpp',
            'package/CommandsExecution.cpp',
            'package/Package.cpp',
            'package/Update.cpp',
            'package/PackageSoftware.cpp',
        ],

        stlibpath=libs,
        stlib    =libs_link,
        libpath  =libs_shared,
        lib      =libs_shared_link,

        use      = [
            'xdelta_objects', 'common_objects', 'includes'
        ]
    ).env.append_unique('DEFINES', packPreprocDefs)

    #
    # Service
    #
    bld.program(
        target='BigKingUpdateMaintainer',
        source=[
            'service/UpdateService.cpp',
            'service/Worker.cpp',
        ],

        stlibpath=libs,
        stlib    =libs_link,
        libpath  =libs_shared,
        lib      =libs_shared_link,

        use      = [
            'common_objects', 'includes'
        ]
    )

    #
    # Self update service
    #
    bld.program(
        target='BigKingUpdateService',
        source=[
            'service/UpdateService.cpp',
            'service/Worker.cpp',
        ],

        stlibpath=libs,
        stlib    =libs_link,
        libpath  =libs_shared,
        lib      =libs_shared_link,

        use      = [
            'common_objects', 'includes'
        ]
    ).env.append_unique('DEFINES', ['BUILD_UPDATE_SERVICE'])
