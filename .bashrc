[[ $- != *i* ]] && return

if [ "$(tty | grep -Z pts)" ]; then
    if ([[ $(ps --no-header -p $PPID -o comm) =~ termite ]] || [[ $(ps --no-header -p $PPID -o comm) =~ alacritty ]]); then
        for wid in $(xdotool search --pid $PPID); do
            xprop -f _KDE_NET_WM_BLUR_BEHIND_REGION 32c -set _KDE_NET_WM_BLUR_BEHIND_REGION 0 -id $wid
        done
    fi

    [ -f ~/.logo.txt ] && cat ~/.logo.txt

    restore() {
        history -a
        $(cat ~/.term) &
        set > "/tmp/.$(whoami).restore"
        alias >> "/tmp/.$(whoami).restore"
        exit
    }
    if [ -f "/tmp/.$(whoami).restore" ]; then
        . "/tmp/.$(whoami).restore" &> /dev/null
        rm -f "/tmp/.$(whoami).restore"
        return
    fi

    gitlastcommit() { git log -1 --pretty=format:%B; }
    gittrackedfiles() {
        find -L * .!(|.|git) -print0 |
        while read -d $'\0' f; do
            git check-ignore -q -- "$f" &> /dev/null
            if [ "$?" -eq 1 ]; then echo "$f"; fi
        done | LC_COLLATE=C sort --ignore-case
    }

    dlytplaylist() {
        local O="$2"
        [ -z "$O" ] && O="."
        yt-dlp --no-warnings --all-subs -x "$1" \
        -o "$O/%(playlist_index)03d. %(title)s.%(ext)s" -o "chapter:$O/%(section_number)03d. %(section_title)s.%(ext)s" \
        --audio-format opus --audio-quality 0 --ignore-config "${@:3}"
    }

    nxdk() {
        eval -- "$("$HOME/.local/share/nxdk/bin/activate" -s)"
    }
    cedev() {
        export PATH="$HOME/.local/bin/CEdev/bin"
    }
    ruby() {
        export PATH="$PATH:$(for f in ~/.local/share/gem/ruby/*/bin/; do echo "$f"; break; done)"
    }
fi

export PATH="$PATH:$HOME/.local/bin"

export WINEDEBUG=-all,-d3d
#export PROMPT_COMMAND='printf "\e]2;$(echo $TERM | sed -E "s/[[:alnum:]_'"\'"'-]+/\u&/g"): $USER@$HOSTNAME: ${PWD##*/}\007"'
export PS1="\[\e[0;1m\e[38;2;255;255;0m\][\[\e[38;2;0;0;255m\]\u\[\e[38;2;255;0;0m\]@\[\e[38;2;0;255;0m\]\h\[\e[38;2;255;255;0m\]]\[\e[38;2;255;0;255m\]:\[\e[38;2;0;255;255m\]\w\[\e[38;2;255;255;225m\]\$\[\e[0m\] "
export QHD_COLORS=''

alias ls='ls --color=auto'
alias ll='ls -lav --ignore=..'
alias l='ls -lav --ignore=.?*'

alias qhd='qhd -Ce'

#alias 'hexdump++'="hexdump -v -e '\"│ %016_ax │ \"' -e ' 4/1 \"%02x \" \"  \" 4/1 \"%02x \" \"  \"  4/1 \"%02x \" \"  \" 4/1 \"%02x \"  ' -e '\" │ \" 16/1 \"% 1_p\" \" │\n\"'"
#alias 'hd'='hexdump'
#alias 'hd+'='hexdump++'
#alias 'hd++'='hexdump++'

[[ -z "$FUNCNEST" ]] && export FUNCNEST=255

killbypath() {
    shopt -s extglob
    if [[ "$#" -lt 1 ]]; then
        echo "Please provide a path to the first argument"
        return 1
    fi
    local kpath="$(readlink -m -- "$1")"
    shift
    local pids=""
    for file in /proc/+([0-9]); do
        local pid="$(echo "$file" | sed 's/\/proc\///')"
        local path="$(readlink -m -- "$file/exe")"
        if [[ ! -z "$path" ]] && [[ "$path" == "$kpath" ]]; then
            pids="$pids $pid"
        fi
    done
    if [[ ! -z "$pids" ]]; then
        kill "$@" -- $pids
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

private() { unset HISTFILE; }
alias priv=private
alias p=private

#alias mdcd='mkdir -p && cd'
mdcd() {
    mkdir -p "$@" && cd "$@"
}

export GPG_TTY=$(tty)

export HISTSIZE=1024
export HISTCONTROL=ignoreboth:erasedups
if [ -f .bash_history_override ]; then
    t="$(stty -g)"
    stty intr ''
    echo -n "Load history override [Y/n]? "
    read -rs -n 1 I
    echo
    stty "$t"
    [[ "$I" = "" || "${I,,}" = y ]] && HISTFILE="$(pwd)/.bash_history_override"
fi
