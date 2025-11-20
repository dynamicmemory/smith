using System;
using Latent.Users;
using Latent.Database;

class App {

    static void Main() {
        var u = new User("Neo", "Administration", 1001);
        u.PrintUser(); 
    }
}



