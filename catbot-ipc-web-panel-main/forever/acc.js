const fs = require('fs');
const path = require('path');
const timestamp = require('time-stamp');

const accounts_dir = path.resolve(__dirname, '..', '..');

function account_file_for_generation(generation) {
    const name = !generation ? 'accounts.txt' : `accounts${generation}.txt`;
    return path.join(accounts_dir, name);
}

function parse_accounts(text) {
    const account_array = [];
    const lines = text.split(/\r\n|\r|\n/g);

    for (const raw_line of lines) {
        const line = raw_line.trim();
        if (!line || line.startsWith('#'))
            continue;

        const separator = line.indexOf(':');
        if (separator <= 0)
            continue;

        account_array.push({
            login: line.slice(0, separator),
            password: line.slice(separator + 1)
        });
    }

    return account_array;
}

module.exports = {
    get: function get(index, generation) {
        generation = Number.parseInt(generation || 0, 10);
        if (!Number.isFinite(generation) || generation < 0)
            generation = 0;

        const account_file = account_file_for_generation(generation);
        try {
            const accounts = fs.readFileSync(account_file, 'utf8');
            const account_array = parse_accounts(accounts);
            const account_index = index > 0 ? index - 1 : index;
            if (account_index < 0 || account_index >= account_array.length)
            {
                console.log(`[${timestamp('HH:mm:ss')}][Account Database] Index ${index} not in ${account_file} (too few accounts)`);
                return null;
            }
            return account_array[account_index];
        }
        catch (error) {
            if (generation > 0 && error && error.code === 'ENOENT') {
                console.error(`[${timestamp('HH:mm:ss')}][Account Database] Missing ${account_file}; bot cannot advance to generation ${generation}.`);
                return null;
            }
            console.error(error);
            console.error(`Error Reading '${account_file}' in catbot-setup. Exiting.`);
            process.exit(1);
        }
    }
}
