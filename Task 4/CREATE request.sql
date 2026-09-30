CREATE DATABASE clients_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

CREATE TABLE clients 
(
    id         BIGINT AUTO_INCREMENT PRIMARY KEY,
    first_name VARCHAR(50) NOT NULL,
    last_name  VARCHAR(50) NOT NULL,
    email      VARCHAR(255) NULL,
    UNIQUE KEY uk_clients_email (email)
) ENGINE=InnoDB;

CREATE TABLE phones 
(
    id        BIGINT AUTO_INCREMENT PRIMARY KEY,
    client_id BIGINT NOT NULL,
    phone     VARCHAR(64) NOT NULL,
    KEY idx_phones_client (client_id),
    CONSTRAINT fk_phones_client FOREIGN KEY (client_id)
        REFERENCES clients(id) ON DELETE CASCADE
) ENGINE=InnoDB;