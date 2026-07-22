const express = require('express');
const router = express.Router();
const auth = require('../middleware/auth');
const { getExecutions, getExecutionById, recordExecution } = require('../controllers/executionController');

router.get('/', auth, getExecutions);
router.get('/:id', auth, getExecutionById);
router.post('/record', recordExecution); // Called internally by worker (no JWT)

module.exports = router;
