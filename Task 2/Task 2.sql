CREATE TABLE employee (
    id          INT          NOT NULL,
    name        VARCHAR(100) NOT NULL,
    department  VARCHAR(100) NOT NULL,
    manager_id  INT          NULL,
    
    CONSTRAINT pk_employee PRIMARY KEY (id),
    CONSTRAINT fk_employee_manager
    FOREIGN KEY (manager_id) REFERENCES employee (id)
    ON DELETE SET NULL
    ON UPDATE CASCADE
);