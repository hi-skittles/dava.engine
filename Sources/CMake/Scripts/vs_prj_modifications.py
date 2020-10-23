#!/usr/bin/env python
import sys  
import glob, os
import re
import argparse
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import fromstring, ElementTree, Element

g_nsLink = '{http://schemas.microsoft.com/developer/msbuild/2003}' 

def load_vcx_tree( pathVcxProj ) :
    ET.register_namespace("","http://schemas.microsoft.com/developer/msbuild/2003")
    tree = ET.parse( pathVcxProj )
    root = tree.getroot()
    return tree, root

def vs_prj_modification_dependent( args ) :
    print 'Added dependency to [ {0} ] in project {1}'.format( args.targetDepend, args.pathVcxProj )
    
    tree, root = load_vcx_tree( args.pathVcxProj )
    modified_project = False

    for neighbor in root.iter( g_nsLink+'ItemGroup' ) :
        for child in neighbor:
            ret = child.attrib.get( 'Include' ).find( args.targetDepend ) 
            if( ret !=  -1 ) :

                find_val = child.findall( g_nsLink+'LinkLibraryDependencies' ) 
                if( find_val ) :
                    if( find_val[0].text == 'false' ) :
                        modified_project = True
                    find_val[0].text = 'true'
                else :
                    node = Element( 'LinkLibraryDependencies' )  
                    node.text = 'true'
                    child.append( node  )
                    modified_project = True
                      
                find_val = child.findall( g_nsLink+'UseLibraryDependencyInputs' ) 
                if( find_val  ) :
                    if( find_val[0].text == 'false' ) :
                        modified_project = True
                    find_val[0].text = 'true'
                else :
                    modified_project = True
                    node = Element( 'UseLibraryDependencyInputs' )  
                    node.text = 'true'
                    child.append( node  )

    if( modified_project ) :
        tree.write( args.pathVcxProj )

    
def vs_uwp_dll_deploy_fix( args ) :
    print 'vs_uwp_dll_deploy_fix in {0}'.format( args.pathVcxProj )
    
    tree, root = load_vcx_tree( args.pathVcxProj )
    modified_project = False

    for neighbor in root.iter( g_nsLink+'ItemGroup' ):
        for child in neighbor:
            #search included dll
            if( child.tag == g_nsLink+'None' and child.attrib.get( 'Include' ).find( 'dll' ) != -1 ):
                #find out target config of library
                words = child.attrib.get( 'Include' ).split('\\')
                words.reverse()
                config = (words[1] + '|' + words[2]).lower()
                
                #mark dll as deployment content only for target config
                for content in child.iter( g_nsLink+'DeploymentContent' ):
                    condition = content.attrib.get( 'Condition' ).lower()
                    if ( condition.find(config) != -1 ):
                        if ( content.text != 'true' ):
                            content.text = 'true'
                            modified_project = True
                    else:
                        if ( content.tag != g_nsLink+'ExcludedFromBuild' or content.text != 'true' ):
                            content.tag = g_nsLink+'ExcludedFromBuild'
                            content.text = 'true'
                            modified_project = True

    if( modified_project == True ):
        tree.write( args.pathVcxProj )
        
    
def vs_prj_dpi_awarness( args ) :
    print 'Set DPI-aware setting to [ {0} ] for {1}'.format( args.typeAwerness, args.pathVcxProj )

    if args.typeAwerness == 'None':
        typeAwerness = 'false'
    elif args.typeAwerness == 'HighDPIAware':
        typeAwerness = 'true'
    elif args.typeAwerness == 'PerMonitorHighDPIAware':
        typeAwerness = args.typeAwerness
    else:
        raise AssertionError( 'Invalid value args.typeAwerness :', args.typeAwerness  )

    tree, root = load_vcx_tree( args.pathVcxProj )
    modified_project = False

    for neighbor in root.iter( g_nsLink+'ItemDefinitionGroup' ):
        manifest = neighbor.find( g_nsLink+'Manifest' ) 

        if manifest == None:
            manifest = Element( 'Manifest' )  
            neighbor.append( manifest  )
            modified_project = True

        enableDpiAwareness = manifest.find(g_nsLink+'EnableDpiAwareness')

        if enableDpiAwareness == None:
            enableDpiAwareness = Element( 'EnableDpiAwareness' )  
            manifest.append( enableDpiAwareness )
            modified_project = True

        if enableDpiAwareness.text != typeAwerness :
            enableDpiAwareness.text = typeAwerness
            modified_project = True

    if( modified_project == True ):
        tree.write( args.pathVcxProj )

        
def main():
    parser = argparse.ArgumentParser()

    subparsers = parser.add_subparsers( help='commands' )

    # vs_prj_modification_dependent
    projDependent_parser = subparsers.add_parser('projDependent', help='vs prj modification dependent')
    projDependent_parser.add_argument( '--pathVcxProj', required = True )
    projDependent_parser.add_argument( '--targetDepend', required = True )
    projDependent_parser.set_defaults( func = vs_prj_modification_dependent )

    # vs_uwp_dll_deploy_fix
    uwpDeployDll_parser = subparsers.add_parser('uwpDeployDll', help='vs uwp dll deploy fix')
    uwpDeployDll_parser.add_argument( '--pathVcxProj', required = True )
    uwpDeployDll_parser.set_defaults( func = vs_uwp_dll_deploy_fix )

    # vs_prj_dpi_awarness
    dpiAwarness_parser = subparsers.add_parser('dpiAwarness', help='dpi awarness')
    dpiAwarness_parser.add_argument( '--pathVcxProj', required = True )
    dpiAwarness_parser.add_argument( '--typeAwerness', required = True, choices=['None', 'PerMonitorHighDPIAware', 'HighDPIAware'] )   
    dpiAwarness_parser.set_defaults( func = vs_prj_dpi_awarness )
    
    args = parser.parse_args()
    args.func( args )                  
    
if __name__ == '__main__':
    main()

