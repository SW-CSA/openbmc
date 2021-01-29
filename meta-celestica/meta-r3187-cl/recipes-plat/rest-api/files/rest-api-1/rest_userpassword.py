#!/usr/bin/python
import os
import sys
import pexpect

def login(user, password):
    child = pexpect.spawnu('login')
    i = child.expect([pexpect.TIMEOUT, 'login', pexpect.EOF], timeout=1)
    if i == 1:
        child.sendline(user)
        i = child.expect([pexpect.TIMEOUT, 'Password', pexpect.EOF], timeout=1)
        if i == 1:
            child.sendline(password)
            child.sendline('whoami')
            i = child.expect([pexpect.TIMEOUT, user, pexpect.EOF], timeout=1)
            if i == 1:
                child.sendline('exit')
                return True
            child.send(chr(3)) # Ctrl-C
    return False

def change_password(user, oldpassword, newpassword):
    ret = False
    child = pexpect.spawnu('useradd {0}'.format(user))
    i = child.expect([pexpect.TIMEOUT, 'already exists', pexpect.EOF], timeout=1)
    if i == 1:
        if not login(user, oldpassword):
            return ret

    child = pexpect.spawnu('passwd {0}'.format(user))
    # i = 0
    # i = child.expect([pexpect.TIMEOUT, '[Pp]assword', pexpect.EOF], timeout=1)
    # if i == 1:
        # child.sendline(oldpassword)
    try:
        # Root does not require old password, so it gets to bypass the next step.
        # i = child.expect([pexpect.TIMEOUT, "Old password", pexpect.EOF], timeout=1)
        # if i == 1:
            # child.sendline(oldpassword)
        i = child.expect([pexpect.TIMEOUT, "New password", pexpect.EOF], timeout=1)
        if i == 1:
            child.sendline(newpassword)
        i = child.expect([pexpect.TIMEOUT, 'Re-enter new password', pexpect.EOF], timeout=1)
        if i == 1:
            child.sendline(newpassword)
        else:
            child.send(chr(3)) # Ctrl-C
            child.sendline('') # This should tell remote passwd command to quit.
        i = child.expect([pexpect.TIMEOUT, 'password changed', pexpect.EOF], timeout=1)
        if i == 1:
            ret = True
            os.popen("cp /etc/passwd /mnt/data/etc/passwd")
            os.popen("cp /etc/shadow /mnt/data/etc/shadow")
    except pexpect.exceptions.ExceptionPexpect:
        pass
    return ret

def userpassword_action(data):
    result = {}
    username = data["user"]
    oldpassword = data["oldpassword"]
    newpassword = data["newpassword"]

    if change_password(username, oldpassword, newpassword):
        result = {"result": "success"}
    else:
        result = {"result": "fail"}

    return result
