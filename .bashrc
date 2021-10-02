[[ $- != *i* ]] && return

if [ "$(tty | grep -Z pts)" ]; then
    if [ -f ~/.logo.txt ]; then cat ~/.logo.txt; fi
    restore() {
        history -a
        $(cat ~/.term) &
        set > "/tmp/.$(whoami).restore"
        exit
    }
    if [ -f "/tmp/.$(whoami).restore" ]; then
        source "/tmp/.$(whoami).restore" &> /dev/null
        rm -f "/tmp/.$(whoami).restore"
        return
    fi
fi

PROMPT_COMMAND='printf "\e]2;$(echo $TERM | sed -E "s/[[:alnum:]_'"\'"'-]+/\u&/g"): $USER@$HOSTNAME: ${PWD##*/}\007"'

export WINEDEBUG=-all
export PS1="\[\e[0;1m\e[38;2;255;255;0m\][\[\e[38;2;0;0;255m\]\u\[\e[38;2;255;0;0m\]@\[\e[38;2;0;255;0m\]\h\[\e[38;2;255;255;0m\]]\[\e[38;2;255;0;255m\]:\[\e[38;2;0;255;255m\]\w\[\e[38;2;255;255;225m\]\$\[\e[0m\] "

alias ls='ls --color=auto'
alias ll='ls -lav --ignore=..'
alias l='ls -lav --ignore=.?*'

[[ -z "$FUNCNEST" ]] && export FUNCNEST=255

scrub() {
    if [ $# -gt 1 ]; then
        echo "Only one file may be scrubbed at a time."
    else
        if [ $# -eq 0 ]; then
            echo "A file name must be provided."
        else
            if [ -f "$@" ]; then
                local bytes="$(stat -c %s "$@")"
                dd if=/dev/random bs=1 count="$bytes" of="$@" &> /dev/null
                dd if=/dev/zero bs=1 count="$bytes" of="$@" &> /dev/null
                rm -f "$@"
            else
                echo "Argument must be a file."
            fi
        fi
    fi
}

#if ([[ $(ps --no-header -p $PPID -o comm) =~ termite ]] || [[ $(ps --no-header -p $PPID -o comm) =~ alacritty ]]); then
#	for wid in $(xdotool search --pid $PPID); do
#		xprop -f _KDE_NET_WM_BLUR_BEHIND_REGION 32c -set _KDE_NET_WM_BLUR_BEHIND_REGION 0 -id $wid
#	done
#fi

PATH="$PATH:~/.local/bin"

export HISTCONTROL=ignoreboth:erasedups
