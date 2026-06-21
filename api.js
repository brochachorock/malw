const express = require('express');
const app = express();
const fs = require('fs');
const path = require('path');

app.use(express.json({ limit: '50mb' }));
app.use(express.urlencoded({ limit: '50mb', extended: true }));

app.post('/a', (req, res) => {
    try {
        const { mouse_x, mouse_y, screenshot } = req.body;

        const base64Data = screenshot.replace(/^data:image\/w+;base64,/,"");
        const imageBuffer = Buffer.from(base64Data, 'base64');

        const fileName = `screenshot_${Date.now()}.png`;
        const fPath = path.join(__dirname, 'capturas', fileName);

        if (!fs.existsSync(path.join(__dirname, 'capturas'))) {
            fs.mkdirSync(path.join(__dirname, 'capturas'));
        }

        fs.writeFile(fPath, imageBuffer, (e) => {
            if (e) {
                console.e('e');
                return res.status(500).send('error');
            }

            console.log('sc saved: ' + fileName);
        })
    } catch (error) {
        res.status(500).send('erro');
    }
});

app.listen(3000, () => {
    console.log('3000.')
})