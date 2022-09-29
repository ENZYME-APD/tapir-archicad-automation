#!/usr/bin/env python27
# -*- coding: utf-8 -*-

__all__ = ['SwissKnife']
import scriptcontext

class SwissKnife:
    """
    Set of tools 
    """
    @staticmethod
    def component_guid(number):
        _HEX = "fa831bc3-fc4a-43b5-9b22-{}"
        # d= int('31973283d01c',base=16) + number 
        sufix= '9184e72a000L'
        # number = hex(number)
        d = hex(int(sufix,16) + number)     
        # print d
        # d = hex(int(sufix, 16) + int(number, 16))     
        # return _HEX.format( d[2:])
        print len('9184e72a001L')
        return _HEX.format( d[2:])
    
    @staticmethod
    def get_wrapped_value(current_index,_list):
        '''
        wraps current_value to fit a lenght of a list
        '''
        wrapped_index = 0
        if len(_list):
            multiplication = int(current_index/len(_list)) 
            wrapped_index = current_index - (multiplication*len(_list))
        return int(wrapped_index)

    @staticmethod
    def show_menu(header_name,current_index,_list):
        '''
            draws menu 
        '''
        header = '''
---------------
{}
---------------
        '''.format(header_name)

        rows=[]
        rows.append(header) 
        for i,j in enumerate(_list):
            
            if i == current_index:
                arrow = '<--pick ({})'.format(j)
            else:
                arrow = ''
            rows.append('{} | {}     {}'.format(i,j,arrow))

        show_content = '\n'.join(rows)
        # print show_content
        return show_content
    @staticmethod
    def save_port(obj):
        try:

            scriptcontext.sticky['link'] = obj
            return scriptcontext.sticky['link']
        except:
            print 'Run outside GH'

if __name__ == '__main__':
    print SwissKnife.component_guid(1)
    print int('5af3107a4001',16)