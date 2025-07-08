public class Ipv6Service {
    public String nodeip;
    public String nodename;
    public String resource;

    public Ipv6Service(String nodeip, String nodename, String resource) {
        this.nodeip = nodeip;
        this.nodename = nodename;
        this.resource = resource;
    }

    @Override
    public String toString() {
        return String.format("[%s] %s - %s", nodeip, nodename, resource);
    }
}
