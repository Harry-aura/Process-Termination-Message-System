const express = require('express');
const cors = require('cors');
const { execSync, spawnSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const app = express();
const PORT = 3000;

app.use(cors());
app.use(express.json());
app.use(express.static('public'));

app.post('/api/run', (req, res) => {
    const { code, cpuLimit = 1, memLimit = 32 } = req.body;
    const timestamp = Date.now();
    const srcPath = path.join(__dirname, `temp_${timestamp}.c`);
    const binPath = path.join(__dirname, `temp_${timestamp}.out`);

    fs.writeFileSync(srcPath, code);

    const gcc = spawnSync('gcc', ['-O2', srcPath, '-o', binPath]);
    if (gcc.status !== 0) {
        if (fs.existsSync(srcPath)) fs.unlinkSync(srcPath);
        return res.json({ success: false, stage: "COMPILATION", error: gcc.stderr.toString() });
    }

    const run = spawnSync('./safeexec_engine', [binPath, String(cpuLimit), String(memLimit)]);
    if (fs.existsSync(srcPath)) fs.unlinkSync(srcPath);
    if (fs.existsSync(binPath)) fs.unlinkSync(binPath);

    let telemetry = {};
    try {
        telemetry = JSON.parse(run.stdout.toString());
    } catch (e) {
        return res.status(500).json({ error: "Failed to parse supervisor output", raw: run.stderr.toString() });
    }

    let zombieCount = 0;
    try {
        const audit = execSync('ps -eo stat | grep -c "^Z" || true').toString().trim();
        zombieCount = parseInt(audit, 10) || 0;
    } catch (e) {
        zombieCount = 0;
    }

    telemetry.zombies_active = zombieCount;
    return res.json({ success: true, stage: "RUNTIME", data: telemetry });
});

app.listen(PORT, () => {
    console.log(`Server online at http://localhost:${PORT}`);
});
