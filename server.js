const express = require('express');
const { Client } = require('pg');
const app = express();
const port = 3000;

// Configuração do middleware para parsing de JSON
app.use(express.json());

// Configuração do cliente PostgreSQL
const client = new Client({
  host: 'localhost',  // Endereço do servidor PostgreSQL
  port: 5432,        // Porta do PostgreSQL
  user: 'postgres', // Seu usuário do PostgreSQL
  password: 'Gt@240396', // Sua senha do PostgreSQL
  database: 'TCC' // Nome do banco de dados
});

// Conectar ao PostgreSQL
client.connect()
  .then(() => {
    console.log('Conectado ao PostgreSQL');
  })
  .catch(err => {
    console.error('Erro ao conectar ao PostgreSQL:', err);
  });

// Endpoint para receber os dados do ESP32
app.post('/endpoint', (req, res) => {
  const { temperatura, batimentos, spo2 } = req.body;

  // Inserir os dados no banco de dados
  const query = 'INSERT INTO public.medidas (temperatura, batimentos, spo2) VALUES ($1, $2, $3)';
  client.query(query, [temperatura, batimentos, spo2], (err, result) => {
    if (err) {
      console.error(err);
      res.status(500).send('Erro ao salvar dados');
    } else {
      res.status(200).send('Dados salvos com sucesso');
    }
  });
});

// Iniciar o servidor na porta 3000
app.listen(port, () => {
  console.log(`Servidor rodando na porta ${port}`);
});
