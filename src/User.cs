namespace Latent.Users {

    public class User {

        public string Name { get; private set; }
        public string Type {get; private set; }
        public int Id { get; private set; }

        public User(string name, string type, int id) {
            Name = name;
            Type = type;
            Id = id;
        }

        public void PrintUser() {
            Console.WriteLine($"User Name: {this.Name}");
            Console.WriteLine($"User Type: {this.Type}");
            Console.WriteLine($"User ID:   {this.Id}");
        }
    }
}
