[[ $- != *i* ]] && return

if [ "$(tty | grep -Z pts)" ]; then
    if [ -f ~/.logo.txt ]; then cat ~/.logo.txt; fi
    restore() {
        history -a
        $(cat ~/.term) &
        set > "/tmp/.$(whoami).restore"
        alias >> "/tmp/.$(whoami).restore"
        exit
    }
    if [ -f "/tmp/.$(whoami).restore" ]; then
        source "/tmp/.$(whoami).restore" &> /dev/null
        rm -f "/tmp/.$(whoami).restore"
        return
    fi
fi

export WINEDEBUG=-all
#export PROMPT_COMMAND='printf "\e]2;$(echo $TERM | sed -E "s/[[:alnum:]_'"\'"'-]+/\u&/g"): $USER@$HOSTNAME: ${PWD##*/}\007"'
export PS1="\[\e[0;1m\e[38;2;255;255;0m\][\[\e[38;2;0;0;255m\]\u\[\e[38;2;255;0;0m\]@\[\e[38;2;0;255;0m\]\h\[\e[38;2;255;255;0m\]]\[\e[38;2;255;0;255m\]:\[\e[38;2;0;255;255m\]\w\[\e[38;2;255;255;225m\]\$\[\e[0m\] "
export QHD_COLORS=''

alias ls='ls --color=auto'
alias ll='ls -lav --ignore=..'
alias l='ls -lav --ignore=.?*'

#alias 'hexdump++'="hexdump -v -e '\"│ %016_ax │ \"' -e ' 4/1 \"%02x \" \"  \" 4/1 \"%02x \" \"  \"  4/1 \"%02x \" \"  \" 4/1 \"%02x \"  ' -e '\" │ \" 16/1 \"% 1_p\" \" │\n\"'"
#alias 'hd'='hexdump'
#alias 'hd+'='hexdump++'
#alias 'hd++'='hexdump++'

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

getcolors() {
    for i in {30..37} ; do
        printf "\e[${i}m██"
    done
    echo
    printf "\e[1m"
    for i in {30..37} ; do
        printf "\e[${i}m██"
    done
    printf "\e[0m\n"
}

stripa() {
    LC_ALL=C awk -F "" '{for(i=1;i<=NF;i++)if(i%4!=p)printf $i}'
}

if ([[ $(ps --no-header -p $PPID -o comm) =~ termite ]] || [[ $(ps --no-header -p $PPID -o comm) =~ alacritty ]]); then
	for wid in $(xdotool search --pid $PPID); do
		xprop -f _KDE_NET_WM_BLUR_BEHIND_REGION 32c -set _KDE_NET_WM_BLUR_BEHIND_REGION 0 -id $wid
	done
fi

export PATH="$PATH:$HOME/.local/bin:$HOME/.local/bin/CEdev/bin"
export HISTCONTROL=ignoreboth:erasedups

#alias mdcd='mkdir -p && cd'
mdcd() {
    mkdir -p "$@" && cd "$@"
}

export GPG_TTY=$(tty)
