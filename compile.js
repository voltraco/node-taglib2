require('shelljs/global');

if (process.env.ELECTRON) {
    exec('npm run compile:electron');
    return;
}

exec('npm run compile');