import os.path
import sys

if __name__=='__main__':
    file = sys.argv[1]
    if not os.path.isfile(file):
        raise Exception( file + "does not exist")
    
    flag_for_comment = False
    flag_for_gpp = False
    command = ''

    with open(file) as f:
        lines= f.readlines()
        for l in lines:
            if '/*' in l:
                flag_for_comment = True
            elif flag_for_comment:
                if 'g++' in l:
                    flag_for_gpp = True
                    command += l
                elif flag_for_gpp:
                    if '\\' in l:
                        command += l
                    else:
                        flag_for_gpp = False
                        command += l
                else:
                    pass
            else:
                pass
    print(command)