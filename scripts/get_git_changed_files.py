import subprocess
import sys

def run_git_command(command):
    """
    Helper function to execute a Git command and return the output.
    """
    try:
        result = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True,
        )
        return result.stdout.strip().split("\n") if result.stdout else []
    except subprocess.CalledProcessError as e:
        print(f"Error running command '{' '.join(command)}': {e.stderr}")
        return []

def get_git_changed_files():
    """
    Get files in various change states in a Git repository.
    """
    # Modified but not staged
    modified_files = run_git_command(['git', 'diff', '--name-only'])

    # Staged for commit
    staged_files = run_git_command(['git', 'diff', '--name-only', '--cached'])

    # Untracked files
    untracked_files = run_git_command(['git', 'ls-files', '--others', '--exclude-standard'])

    return {
        'modified': modified_files,
        'staged': staged_files,
        'untracked': untracked_files,
    }

if __name__ == '__main__':
    changed_files = get_git_changed_files()
    if (len(changed_files['modified']) > 0 or len(changed_files['staged']) > 0 or len(changed_files['untracked']) > 0):
        print('Modified files (not staged):', changed_files['modified'])
        print('Staged files:', changed_files['staged'])
        print('Untracked files:', changed_files['untracked'])
        sys.exit(1)